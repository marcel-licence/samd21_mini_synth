/*
 * The GNU GENERAL PUBLIC LICENSE (GNU GPLv3)
 *
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
 * @file cdt.h
 * @author Marcel Licence
 * @date 13.05.2021
 *
 * @brief This file helps to get all required defines in another IDE
 */


#ifndef CDT_H_
#define CDT_H_

#include <config.h>

#ifdef __CDT_PARSER__
#define PROGMEM
#define uint8_t unsigned char
#define int8_t char
#define int16_t short
#define uint32_t unsigned int
#define uint64_t unsigned long
#define uint16_t unsigned short
#define int32_t int
#define byte char
#define ICACHE_RAM_ATTR
#define IRAM_ATTR
#define pow(a,b)    (a^b)
#define NULL ((void*)0)
#define PI  3.13f
extern void delay(int a);
class cSerial
{
    void begin(int);
    void println(void);
    void printf(const char *str, ...);
};
extern void btStop(void);
extern int max(int a, int b);
extern class cSerial Serial;
extern void memset(void *a, int b, int c);
extern int millis(void);
extern int random(int val);
extern void *malloc(int val);
extern void memcpy(void *a, void *b, int size);
extern void randomSeed(int a);
extern void sin(float f);
#include <esp32_basic_synth.ino>
#include <esp32_fm_synth.ino>
#include <blink.ino>
#include <display_i2c.ino>
#include <easySynth.ino>
#include <es8388.ino>
#include <esp32_audio_kit_module.ino>
#include <fm_synth_module.ino>
#include <i2s_interface.ino>
#include <launchpad_ifc.ino>
#include <led_module.ino>
#include <reverb.ino>
#include <sampler_module.ino>
#include <sequencer.ino>
#include <screens.ino>
#include <simple_delay.ino>
#include <sine.ino>
#include <status_module.ino>
#include <usbMidiHost.ino>
#include <z_config.ino>
#endif

#endif /* CDT_H_ */
