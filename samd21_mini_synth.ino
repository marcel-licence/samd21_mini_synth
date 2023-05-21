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
 * @file samd21_mini_synth.ino
 * @author Marcel Licence
 * @date 22.10.2021
 *
 * @brief This file contains the project code of "Lo-Fi SAMD21 based mini chip tune synthesizer - Seeed Studio - Seeeduino XIAO arduino project"
 * Schmatic: https://files.seeedstudio.com/wiki/Seeeduino-XIAO/res/Seeeduino-XIAO-v1.0-SCH-191112.pdf
 * Ref Manual: https://ww1.microchip.com/downloads/en/DeviceDoc/SAM-D21DA1-Family-Data-Sheet-DS40001882G.pdf
 *
 * @see https://youtu.be/uwIEd5NU29I
 */


#ifdef __CDT_PARSER__
#include "cdt.h"
#endif


#include "config.h"


#ifdef MIDI_USB_HOST_ENABLED
#include <Usb.h>
#include <usbh_midi.h>
#include <usbhub.h>
#endif


#define ML_SYNTH_INLINE_DECLARATION
#include <ml_inline.h>
#undef ML_SYNTH_INLINE_DECLARATION


#define MUL_I16(a, b) ((int16_t)((((int32_t)a) * ((int32_t)(b))) / 0x8000))


void setup()
{
    pinMode(7, INPUT_PULLUP);

    pinMode(DAC0, OUTPUT);

#if defined MIDI_USB_HOST_ENABLED
    MidiHost_setup();
#elif defined KEYB_USB_HOST_ENABLED
    KeybHost_setup();
#elif defined MIDI_USB_DEVICE_ENABLED
    MidiDev_setup();
#else
    delay(1000);
    Serial.begin(11500);
    delay(1000);
    Serial.printf("SerUSB Started!\n");
#endif

#if 1
    Midi_Setup();
    Synth_Init();
#ifdef SIMPLE_DELAY_BUFFER_SIZE
    SimpleDelay_Init();
#endif
    SAMD21_Synth_Init();
#endif

    /* play 440 Hz note */
    //Synth_NoteOn(0, 69, 1);

    Serial.printf("SetupDone!\n");
}

static uint32_t cnt = 0;

inline
void ProcessAudio2(uint16_t *buff, size_t len)
{
    int32_t u32buf[SAMPLE_BUFFER_SIZE];
    Synth_Process_Buff(u32buf, len);

#ifdef SIMPLE_DELAY_BUFFER_SIZE
    SimpleDelay_Process(u32buf, len);
#endif

#if 1
    /* convert from u16 to u10 */
    for (size_t i = 0; i < len; i++)
    {
        buff[i] = (uint16_t)(0x200 + (u32buf[i] / 512));
    }
#endif
    cnt += SAMPLE_BUFFER_SIZE;
    Midi_Process();

#ifdef KEYB_USB_HOST_ENABLED
    KeybHost_loop();
#endif
}

void loop_1Hz()
{
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
}

void loop()
{
    if (cnt >= SAMPLE_RATE)
    {
        cnt = 0;
        loop_1Hz();
    }

    SAMD21_Synth_Process(ProcessAudio2);
}
