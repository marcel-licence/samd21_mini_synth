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
 * Dieses Programm ist Freie Software: Sie k�nnen es unter den Bedingungen
 * der GNU General Public License, wie von der Free Software Foundation,
 * Version 3 der Lizenz oder (nach Ihrer Wahl) jeder neueren
 * ver�ffentlichten Version, weiter verteilen und/oder modifizieren.
 *
 * Dieses Programm wird in der Hoffnung bereitgestellt, dass es n�tzlich sein wird, jedoch
 * OHNE JEDE GEW�HR,; sogar ohne die implizite
 * Gew�hr der MARKTF�HIGKEIT oder EIGNUNG F�R EINEN BESTIMMTEN ZWECK.
 * Siehe die GNU General Public License f�r weitere Einzelheiten.
 *
 * Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
 * Programm erhalten haben. Wenn nicht, siehe <https://www.gnu.org/licenses/>.
 */

/**
 * @file config.h
 * @author Marcel Licence
 * @date 19.10.2021
 *
 * @brief This file contains the project configuration
 *
 * All definitions are visible in the entire project
 */


#ifndef CONFIG_H_
#define CONFIG_H_


#pragma GCC optimize ("-O2") /* a bit of fairy dust for the compiler */

//#define MIDI_USB_HOST_ENABLED /* requires usb=arduino */
//#define MIDI_USB_DEVICE_ENABLED /* requires usb=tinyusb */


#define MIDI_RECV_FROM_SERIAL
#define Status_ValueChangedFloat(...)
#define Status_ValueChangedFloatArr(...)
#define SAMPLE_RATE 44100
#define SAMPLE_BUFFER_SIZE  100
#define DELAY_BUFFER_SIZE   6000


#endif /* CONFIG_H_ */
