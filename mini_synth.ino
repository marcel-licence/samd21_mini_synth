/*
 * Copyright (c) 2021 Marcel Licence
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Dieses Programm ist Freie Software: Sie können es unter den Bedingungen
 * der GNU General Public License, wie von der Free Software Foundation,
 * Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
 * veröffentlichten Version, weiter verteilen und/oder modifizieren.
 *
 * Dieses Programm wird in der Hoffnung bereitgestellt, dass es nützlich sein wird, jedoch
 * OHNE JEDE GEWÄHR,; sogar ohne die implizite
 * Gewähr der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
 * Siehe die GNU General Public License für weitere Einzelheiten.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <https://www.gnu.org/licenses/>.
 */

/**
 * @file mini_synth.ino
 * @author Marcel Licence
 * @date 22.10.2021
 *
 * @brief This file contains a simple synthesizer sound generation implementation
 *
 * @bug pitch bend depth / modulation pitch does not work
 *
 * @see https://youtu.be/uwIEd5NU29I
 */


#ifdef __CDT_PARSER__
#include "cdt.h"
#endif


/*
 * Param indices for Synth_SetParam function
 */
#define SYNTH_PARAM_VEL_ENV_ATTACK  0
#define SYNTH_PARAM_VEL_ENV_DECAY   1
#define SYNTH_PARAM_VEL_ENV_SUSTAIN 2
#define SYNTH_PARAM_VEL_ENV_RELEASE 3
#define SYNTH_PARAM_FIL_ENV_ATTACK  4
#define SYNTH_PARAM_FIL_ENV_DECAY   5
#define SYNTH_PARAM_FIL_ENV_SUSTAIN 6
#define SYNTH_PARAM_FIL_ENV_RELEASE 7
#define SYNTH_PARAM_WAVEFORM_1      8
#define SYNTH_PARAM_WAVEFORM_2      9

#define SYNTH_PARAM_MAIN_FILT_CUTOFF    10
#define SYNTH_PARAM_MAIN_FILT_RESO      11
#define SYNTH_PARAM_VOICE_FILT_RESO     12
#define SYNTH_PARAM_VOICE_NOISE_LEVEL   13

#define SYNTH_PARAM_PITCH_BEND_RANGE    14
#define SYNTH_PARAM_MODULATION_SPEED    15
#define SYNTH_PARAM_MODULATION_PITCH    16

#define BENDING_ENABLED
#define PITCH_BEND_MAX  0x400
#define PITCH_BEND_NTRL 0x200

/* lower value prevents getting unwanted distortion */
#define MAX_POLY_OSC    (12) /* osc polyphony, always active reduces single voices poly alot */
#define MAX_POLY_VOICE  12 /* max single voices, can use multiple osc */

/*
 * this is just a kind of magic to go through the waveforms
 * - WAVEFORM_BIT sets the bit length of the pre calculated waveforms
 */
#define WAVEFORM_BIT    8UL
#define WAVEFORM_CNT    (1<<WAVEFORM_BIT)
#define WAVEFORM_Q4     (1<<(WAVEFORM_BIT-2))
#define WAVEFORM_MSK    ((1<<WAVEFORM_BIT)-1)
#define WAVEFORM_I(i)   (((i) >> (32 - WAVEFORM_BIT)) & WAVEFORM_MSK)


#define MIDI_NOTE_CNT 128
uint32_t midi_note_to_add[MIDI_NOTE_CNT]; /* lookup to playback waveforms with correct frequency */
uint32_t midi_note_to_add50c[MIDI_NOTE_CNT]; /* lookup for detuning */

#define SAW_CNT 14

uint8_t sawSelect = 0;

float sinef[WAVEFORM_CNT];
int16_t sine[WAVEFORM_CNT];
int16_t saw[WAVEFORM_CNT];
int16_t square[WAVEFORM_CNT];
int16_t pulse[WAVEFORM_CNT];
int16_t tri[WAVEFORM_CNT];
int16_t noise[WAVEFORM_CNT];
int16_t silence[WAVEFORM_CNT];

int16_t *waveFormList[] =
{
    sine, saw, square, pulse, tri, noise
};

#define WAVEFORM_TYPE_COUNT ((sizeof(waveFormList)/sizeof(waveFormList[0])))

uint16_t PitchBendMult[256];

struct adsrT
{
    float a;
    float d;
    float s;
    float r;
};

struct adsrT adsr_vol = {0.25f, 0.25f, 1.0f, 0.01f};
struct adsrT adsr_fil = {1.0f, 0.25f, 1.0f, 0.01f};


typedef enum
{
    attack, decay, sustain, release
} adsr_phaseT;

/* this prototype is required .. others not -  i still do not know what magic arduino is doing */
inline bool ADSR_Process(const struct adsrT *ctrl, float *ctrlSig, adsr_phaseT *phase);

#define MODULATION_DEPTH_MAX    0x4000
#define MODULATION_PITCH_MAX    0x10
#define MODULATION_SPEED_NORM   32

struct channelValues_s
{
    struct
    {
        uint16_t depth;
        uint16_t pitch;
        int16_t *waveForm;
        uint16_t speed;
    } modulation;

    struct
    {
        uint16_t ctrl;
        uint16_t pitch;
    } pitchBend;

    int16_t *waveForm;

    struct
    {
        float detune;
        uint8_t count;
    } unison;
};

#define MIDI_CH_CNT 16
struct channelValues_s chControl[16];

void Synth_ChannelInit(struct channelValues_s *chCtrl)
{
    chCtrl->modulation.depth = 0;
    chCtrl->modulation.pitch = 1;
    chCtrl->modulation.waveForm = square;
    chCtrl->modulation.speed = MODULATION_SPEED_NORM;

    chCtrl->pitchBend.ctrl = 128;
    chCtrl->pitchBend.pitch = 1;

    chCtrl->waveForm = sine;

    chCtrl->unison.count = 0;
    chCtrl->unison.detune = 0.1;
}

void Synth_AllChannelCtrlReset(void)
{
    for (int i = 0; i < MIDI_CH_CNT; i++)
    {
        Synth_ChannelInit(&chControl[i]);
    }
}

struct oscillatorT
{
    int16_t *waveForm;
    int16_t *dest;
    uint32_t samplePos;
    uint32_t addVal;
};

int16_t voiceSink[SAMPLE_BUFFER_SIZE];
struct oscillatorT oscPlayer[MAX_POLY_OSC];

uint32_t osc_act = 0;

struct notePlayerT
{
    int16_t lastSample[SAMPLE_BUFFER_SIZE];

    uint16_t velocity;
    bool active;
    adsr_phaseT phase;

    uint8_t midiNote;
    uint8_t midiCh;

    int16_t control_sign[SAMPLE_BUFFER_SIZE];
    int16_t out_level;

    adsr_phaseT f_phase;
};


struct notePlayerT voicePlayer[MAX_POLY_VOICE];

uint32_t voc_act = 0;

uint8_t actCh = 0;


#define FLOAT_TO_U16(a)  ((((a)+1.0f)*0.5f) * 0x8000)
#define FLOAT_TO_I16(a)  ((a) * 0x7FFF)

void Synth_Init()
{
    randomSeed(34547379);

    Synth_AllChannelCtrlReset();

    /*
     * let us calculate some waveforms
     * - using lookup tables can save a lot of processing power later
     * - but it does consume memory
     */
    for (int i = 0; i < WAVEFORM_CNT; i++)
    {
        float val = (float)sin(i * 2.0 * PI / WAVEFORM_CNT);
        sinef[i] = (val);
        sine[i] = FLOAT_TO_I16(val);
        saw[i] = FLOAT_TO_I16((2.0f * ((float)i) / ((float)WAVEFORM_CNT)) - 1.0f);
        square[i] =  FLOAT_TO_I16((i > (WAVEFORM_CNT / 2)) ? 1 : -1);
        pulse[i] =  FLOAT_TO_I16((i > (WAVEFORM_CNT / 4)) ? 1 : -1);
        tri[i] = FLOAT_TO_I16(((i > (WAVEFORM_CNT / 2)) ? (((4.0f * (float)i) / ((float)WAVEFORM_CNT)) - 1.0f) : (3.0f - ((4.0f * (float)i) / ((float)WAVEFORM_CNT)))) - 2.0f);
        noise[i] =  FLOAT_TO_I16((random(1024) / 512.0f) - 1.0f);
        silence[i] =  FLOAT_TO_I16(0);
    }

    /*
     * initialize all oscillators
     */
    for (int i = 0; i < MAX_POLY_OSC; i++)
    {
        oscillatorT *osc = &oscPlayer[i];
        osc->waveForm = silence;
        osc->samplePos = (uint32_t)random(1 << 31);
        osc->dest = voiceSink;
    }

    /*
     * initialize all voices
     */
    for (int i = 0; i < MAX_POLY_VOICE; i++)
    {
        notePlayerT *voice = &voicePlayer[i];
        voice->active = false;
        memset(voice->lastSample, 0, sizeof(voice->lastSample));
    }

    /*
     * prepare lookup for constants to drive oscillators
     */
    for (int i = 0; i < MIDI_NOTE_CNT; i++)
    {
        float f = ((pow(2.0f, (float)(i - 69) / 12.0f) * 440.0f));
        uint32_t add = (uint32_t)(f * ((float)(1ULL << 32ULL) / ((float)SAMPLE_RATE)));
        //Serial.printf("f[%d] = %0.3f -> %d\n", i, f, add);
        midi_note_to_add[i] = add;

        /* filling the table which will be used for detuning */
        float f1 = (pow(2.0f, ((float)(i - 69) + 0.5f) / 12.0f) * 440.0f);
        float f2 = (pow(2.0f, ((float)(i - 69) - 0.5f) / 12.0f) * 440.0f);

        midi_note_to_add50c[i] = (uint32_t)((f1 - f2) * ((float)(1ULL << 32ULL) / ((float)SAMPLE_RATE)));
    }

    /*
     * generate lookup for pitch bend
     */
    for (int i = 0; i < 256; i++)
    {
        PitchBendMult[i] = (((float)PITCH_BEND_NTRL) * powf(2.0f, (2 * ((float)(i - 128) / 128.0f)) / 12.0f)); /* range from 0x2000 up to 0x8000 */
    }
}

/*
 * very bad and simple implementation of ADSR
 * - but it works for the start
 */
inline bool ADSR_Process(const struct adsrT *ctrl, float *ctrlSig, adsr_phaseT *phase)
{
    for (int i = 0; i < SAMPLE_BUFFER_SIZE; i++)
    {
        switch (*phase)
        {
        case attack:
            ctrlSig[i] += ctrl->a;
            if (ctrlSig[i] > 1.0f)
            {
                ctrlSig[i] = 1.0f;
                *phase = decay;
            }
            break;
        case decay:
            ctrlSig[i] -= ctrl->d;
            if (ctrlSig[i] < ctrl->s)
            {
                ctrlSig[i] = ctrl->s;
                *phase = sustain;
            }
            break;
        case sustain:
            break;
        case release:
            ctrlSig[i] -= ctrl->r;
            if (ctrlSig[i] < 0.0f)
            {
                ctrlSig[i] = 0.0f;
                //voice->active = false;
                return false;
            }
        }
    }
    return true;
}

void Voice_Off(uint32_t i)
{
    notePlayerT *voice = &voicePlayer[i];
    voice->active = false;
    for (int f = 0; f < MAX_POLY_OSC; f++)
    {
        oscillatorT *osc = &oscPlayer[f];
        if (osc->dest == voice->lastSample)
        {
            osc->dest = voiceSink;
            osc_act -= 1;
        }
    }
    voc_act -= 1;
}

static uint32_t ModCnt = 0;

inline
int16_t GetModulation(void)
{
    ModCnt += (chControl[0].modulation.speed * 7 * (97391 / MODULATION_SPEED_NORM) * SAMPLE_BUFFER_SIZE);
    int32_t sig = chControl[0].modulation.waveForm[WAVEFORM_I(ModCnt)];
    sig *= chControl[0].modulation.depth;
    sig /= 1024;
    sig /= MODULATION_DEPTH_MAX;
    sig += 128;
    return sig;
}

[[gnu::noinline, gnu::optimize("fast-math")]]
inline void Synth_Process_Buff(int32_t *left, int buffLen)
{
    for (int n = 0; n < buffLen; n++)
    {
        left[n] = n;
    }

    /*
     * update pitch bending / modulation
     */

    // PitchBendMult
    uint16_t bend = chControl[0].pitchBend.ctrl;

    uint32_t pitchMultiplier = PitchBendMult[bend & 0xFF] * PitchBendMult[GetModulation() & 0xFF] * chControl[0].modulation.pitch / PITCH_BEND_NTRL;
    //pitchMultiplier = PitchBendMult[128];


    for (int i = 0; i < MAX_POLY_OSC; i++)
    {
        oscillatorT *osc = &oscPlayer[i];
        if (osc->waveForm == NULL)
        {
            osc->waveForm = square;
        }
    }

    /* counter required to optimize processing */

    for (int n = 0; n < buffLen; n++)
    {
        /*
         * destination for unused oscillators
         */
        voiceSink[n] = 0;
    }

    /*
     * oscillator processing -> mix to voice
     */
    for (int i = 0; i < MAX_POLY_OSC; i++)
    {
        oscillatorT *osc = &oscPlayer[i];
        {
            for (int n = 0; n < buffLen; n++)
            {
                osc->samplePos += ((pitchMultiplier * (osc->addVal / PITCH_BEND_NTRL)));
                int16_t sig = osc->waveForm[WAVEFORM_I(osc->samplePos)];
                osc->dest[n] += sig;

                if (osc->dest != voiceSink)
                {
                    left[n] += sig;
                }
            }
        }
    }
}

/*
 * evaluate if oscillator is free
 */
inline bool oscillatorIsFree(struct oscillatorT *osc)
{
    return (osc->dest == voiceSink);
}

/*
 * returns a free osc or NULL
 */
struct oscillatorT *getFreeOsc()
{
    for (int i = 0; i < MAX_POLY_OSC ; i++)
    {
        if (oscillatorIsFree(&oscPlayer[i]))
        {
            return &oscPlayer[i];
        }
    }
    return NULL;
}

struct notePlayerT *getFreeVoice()
{
    for (int i = 0; i < MAX_POLY_VOICE ; i++)
    {
        if (voicePlayer[i].active == false)
        {
            return &voicePlayer[i];
        }
    }
    return NULL;
}


inline void Synth_NoteOn(uint8_t ch, uint8_t note, float vel)
{
    struct notePlayerT *voice = getFreeVoice();
    struct oscillatorT *osc = getFreeOsc();

    actCh = ch;

    /*
     * No free voice found, return otherwise crash xD
     */
    if ((voice == NULL) || (osc == NULL))
    {
        //Serial.printf("voc: %d, osc: %d\n", voc_act, osc_act);
        return ;
    }
    voice->midiCh = ch;
    voice->midiNote = note;
    voice->velocity = 0.25; /* just something to test */
    memset(voice->lastSample, 0, sizeof(voice->lastSample));
    memset(voice->control_sign, 0, sizeof(voice->control_sign));


    if (adsr_fil.a < adsr_fil.s)
    {
        adsr_fil.a = adsr_fil.s;
    }
    voice->f_phase = decay;
    voice->active = true;
    voice->phase = attack;

    voc_act += 1;

    /*
     * add oscillator
     */
    osc->addVal = midi_note_to_add[note];
    osc->waveForm = chControl[ch].waveForm;

    osc->dest = voice->lastSample;

    osc_act += 1;
}

inline void Synth_NoteOff(uint8_t ch, uint8_t note)
{
    for (int i = 0; i < MAX_POLY_VOICE ; i++)
    {
        if ((voicePlayer[i].active) && (voicePlayer[i].midiNote == note) && (voicePlayer[i].midiCh == ch))
        {
            Voice_Off(i);
            voicePlayer[i].phase = release;
        }
    }
}

void Synth_ModulationWheel(uint8_t ch, float value)
{
    chControl[ch].modulation.depth = (value * (float)MODULATION_DEPTH_MAX);
}

void Synth_ModulationSpeed(uint8_t ch, float value)
{
#if 0
    modulationSpeed = value * 10;
    Status_ValueChangedFloat("ModulationSpeed", modulationSpeed);
#endif
}

void Synth_ModulationPitch(uint8_t ch, float value)
{
#if 0
    modulationPitch = value * 5;
    Status_ValueChangedFloat("ModulationDepth", modulationPitch);
#endif
}

void Synth_PitchBend(uint8_t ch, float bend)
{
    if (bend > 2.0f)
    {
        bend = 2.0f;
    }
    if (bend < -2.0f)
    {
        bend = -2.0f;
    }
    bend *= 32.0f;
    bend += 128.0f;
    chControl[ch].pitchBend.ctrl = bend;
}

void Synth_SetParam(uint8_t param, float value)
{
    switch (param)
    {
    case SYNTH_PARAM_VEL_ENV_ATTACK:
        adsr_vol.a = (0.00005 * pow(5000, 1.0f - value));
        Serial.printf("voice volume attack: %0.6f\n", adsr_vol.a);
        break;
    case SYNTH_PARAM_VEL_ENV_DECAY:
        adsr_vol.d = (0.00005 * pow(5000, 1.0f - value));
        Serial.printf("voice volume decay: %0.6f\n", adsr_vol.d);
        break;
    case SYNTH_PARAM_VEL_ENV_SUSTAIN:
        adsr_vol.s = (0.01 * pow(100, value));
        Serial.printf("voice volume sustain: %0.6f\n", adsr_vol.s);
        break;
    case SYNTH_PARAM_VEL_ENV_RELEASE:
        adsr_vol.r = (0.0001 * pow(100, 1.0f - value));
        Serial.printf("voice volume release: %0.6f\n", adsr_vol.r);
        break;

    case SYNTH_PARAM_FIL_ENV_ATTACK:
#if 0
        adsr_fil.a = (0.00005 * pow(5000, 1.0f - value));
#else
        adsr_fil.a = value;
#endif
        Serial.printf("voice filter attack: %0.6f\n", adsr_fil.a);
        break;
    case SYNTH_PARAM_FIL_ENV_DECAY:
        adsr_fil.d = (0.00005 * pow(5000, 1.0f - value));
        Serial.printf("voice filter decay: %0.6f\n", adsr_fil.d);
        break;
    case SYNTH_PARAM_FIL_ENV_SUSTAIN:
        adsr_fil.s = value;
        Serial.printf("voice filter sustain: %0.6f\n", adsr_fil.s);
        break;
    case SYNTH_PARAM_FIL_ENV_RELEASE:
        adsr_fil.r = (0.0001 * pow(100, 1.0f - value));
        Serial.printf("voice filter release: %0.6f\n", adsr_fil.r);
        break;

    case SYNTH_PARAM_WAVEFORM_1:
        {
            uint8_t selWaveForm = (value) * (WAVEFORM_TYPE_COUNT);
            chControl[actCh].waveForm = waveFormList[selWaveForm];
        }
        break;

    case SYNTH_PARAM_WAVEFORM_2:
        {
            uint8_t selWaveForm = (value) * (WAVEFORM_TYPE_COUNT);
            chControl[actCh].modulation.waveForm = waveFormList[selWaveForm];
        }
        break;

    case SYNTH_PARAM_PITCH_BEND_RANGE:
        chControl[actCh].modulation.pitch = (value) * MODULATION_PITCH_MAX;
        break;

    case SYNTH_PARAM_MODULATION_SPEED:
        chControl[0].modulation.speed = value * 128;
        break;

    case SYNTH_PARAM_MODULATION_PITCH:
        chControl[actCh].modulation.pitch = (value) * MODULATION_PITCH_MAX;
        break;

    default:
        /* not connected */
        break;
    }
}

