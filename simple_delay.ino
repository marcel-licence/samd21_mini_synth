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
 * @file simple_delay.ino
 * @author Marcel Licence
 * @date 22.10.2021
 *
 * @brief This file contains the implementation of a very simple and buggy delay
 * - level adjustable
 * - feedback
 * - length adjustable
 * - sometimes very noisy xD
 */


#ifdef __CDT_PARSER__
#include <cdt.h>
#endif


/* max delay can be changed but changes also the memory consumption */
#ifndef DELAY_BUFFER_SIZE
#define DELAY_BUFFER_SIZE   SAMPLE_RATE /* results in one second long delay */
#endif

/*
 * module variables
 */
int16_t delayLine[DELAY_BUFFER_SIZE];

int16_t delayToMix = 0;
int16_t delayFeedback = 0;
uint32_t delayLen = DELAY_BUFFER_SIZE - 2;
uint32_t delayIn = 0;
uint32_t delayOut = 0;

void Delay_Init(void)
{
    Delay_Reset();
}


//#define FLOAT_TO_I16(a) (a * ((float)0x8000))

#define MUL_I16_TO_I32(a, b) (((((int32_t)a) * ((int32_t)(b))) ))

void Delay_Reset(void)
{
    for (int i = 0; i < DELAY_BUFFER_SIZE; i++)
    {
        delayLine[i] = 0;

    }
}

void Delay_Process(int32_t *signal, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        int16_t sig = (int16_t)(((int32_t)signal[i]) / ((int32_t) 0x100));

        delayLine[delayIn] = sig;

        delayOut = delayIn + (1 + DELAY_BUFFER_SIZE - delayLen);

        if (delayOut >= DELAY_BUFFER_SIZE)
        {
            delayOut -= DELAY_BUFFER_SIZE;
        }

        sig = delayLine[delayOut];

        signal[i] += MUL_I16_TO_I32(sig, delayToMix);

        delayLine[delayIn] += MUL_I16(sig, delayFeedback);

        delayIn ++;

        if (delayIn >= DELAY_BUFFER_SIZE)
        {
            delayIn = 0;
        }
    }
}

void Delay_SetFeedback(uint8_t unused, float value)
{
    delayFeedback = FLOAT_TO_I16(value);
    Serial.printf("delay feedback: %0.3f\n", value);
}

void Delay_SetLevel(uint8_t unused, float value)
{
    delayToMix = FLOAT_TO_I16(value) / 0x100;
    Serial.printf("delay level: %0.3f\n", value);
}

void Delay_SetLength(uint8_t unused, float value)
{
    delayLen = (uint32_t)(((float)DELAY_BUFFER_SIZE - 1.0f) * value);
    Serial.printf("delay length: %0.3fms\n", delayLen * (1000.0f / ((float)SAMPLE_RATE)));
}
