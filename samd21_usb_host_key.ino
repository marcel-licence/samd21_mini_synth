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
 * @date 26.10.2021
 *
 * @brief This file contains the implementation to use an USB keyboard to control the synthesizer
 *
 * @see https://youtu.be/jpAP7pvv1PA
 * @see https://youtu.be/uwIEd5NU29I
 */


#ifdef __CDT_PARSER__
#include "cdt.h"
#endif


#ifdef KEYB_USB_HOST_ENABLED

#include <KeyboardController.h>


#define SerialDebug Serial1


USBHost UsbH;
KeyboardController keyboard(UsbH);


void KeybHost_setup()
{
    //bFirst = true;
    //vid = pid = 0;

    if (UsbH.Init())
    {
        SerialDebug.println("USB host did not start");
        while (1); //halt
    }
    SerialDebug.println("USB Host started");

    delay(200);


}

uint32_t lastUSBstate = 0;

void KeybHost_loop()
{
    UsbH.Task();

    uint32_t currentUSBstate = UsbH.getUsbTaskState();
    if (lastUSBstate != currentUSBstate)
    {
        SerialDebug.print("USB state changed: 0x");
        SerialDebug.print(lastUSBstate, HEX);
        SerialDebug.print(" -> 0x");
        SerialDebug.println(currentUSBstate, HEX);
        switch (currentUSBstate)
        {
        case USB_ATTACHED_SUBSTATE_SETTLE:
            SerialDebug.println("Device Attached");
            break;
        case USB_DETACHED_SUBSTATE_WAIT_FOR_DEVICE:
            SerialDebug.println("Detached, waiting for Device");
            break;
        case USB_ATTACHED_SUBSTATE_RESET_DEVICE:
            SerialDebug.println("Resetting Device");
            break;
        case USB_ATTACHED_SUBSTATE_WAIT_RESET_COMPLETE:
            SerialDebug.println("Reset complete");
            break;
        case USB_STATE_CONFIGURING:
            SerialDebug.println("USB Configuring");
            break;
        case USB_STATE_RUNNING:
            SerialDebug.println("USB Running");
            break;
        }
        lastUSBstate = currentUSBstate;
    }
}


/*
 * Keyboard stuff
 */
void printKey();


struct
{
    uint8_t key;
    uint8_t note;
} keyMidiMapping[] =
{
    {4, 26},
    {5, 57},
    {6, 43},
    {7, 40},
    {8, 37},
    {9, 47},
    {10, 54},
    {11, 61},
    {12, 72},
    {13, 68},
    {14, 75},
    {15, 82},
    {16, 71},
    {17, 64},
    {18, 79},
    {19, 86},
    {20, 23},
    {21, 44},
    {22, 33},
    {23, 51},
    {24, 65},
    {25, 50},
    {26, 30},
    {27, 36},
    {28, 58},
    {29, 29},
    {30, 20},
    {31, 27},
    {32, 34},
    {33, 41},
    {34, 48},
    {35, 55},
    {36, 62},
    {37, 69},
    {38, 76},
    {39, 83},

    {45, 90},
    {46, 97},
    {47, 93},
    {48, 100},

    {50, 103},
    {51, 89},
    {52, 96},

    {54, 78},
    {55, 85},
    {56, 92},
};

struct
{
    uint8_t key;
    uint8_t cc;
} keyMidiCcMapping[] =
{
    /* F1 - F12 */
    {58, 58},
    {59, 59},
    {60, 60},
    {61, 61},
    {62, 62},
    {63, 63},
    {64, 64},
    {65, 65},
    {66, 66},
    {67, 67},
    {68, 68},
    {69, 69},

    {79, 79},
    {80, 80},
    {81, 81},
    {82, 82},
};

#define MAP_SIZE (sizeof(keyMidiMapping)/sizeof(keyMidiMapping[0]))
#define MAP_CC_SIZE (sizeof(keyMidiCcMapping)/sizeof(keyMidiCcMapping[0]))


// This function intercepts key press
void keyPressed()
{
    SerialDebug.print("Pressed:  ");
    printKey();
    for (int i = 0; i < MAP_SIZE; i++)
    {
        if (keyMidiMapping[i].key == keyboard.getOemKey())
        {
            Midi_NoteOn(0, keyMidiMapping[i].note, 127);
        }
    }

    for (int i = 0; i < MAP_CC_SIZE; i++)
    {
        if (keyMidiCcMapping[i].key == keyboard.getOemKey())
        {
            Midi_ControlChange(0, keyMidiCcMapping[i].cc, 127);
            SerialDebug.printf("cc: %d on\n", keyMidiCcMapping[i].cc);
        }
    }
}

// This function intercepts key release
void keyReleased()
{
    SerialDebug.print("Released: ");
    printKey();

    for (int i = 0; i < MAP_SIZE; i++)
    {
        if (keyMidiMapping[i].key == keyboard.getOemKey())
        {
            Midi_NoteOff(0, keyMidiMapping[i].note);
        }
    }

    for (int i = 0; i < MAP_CC_SIZE; i++)
    {
        if (keyMidiCcMapping[i].key == keyboard.getOemKey())
        {
            Midi_ControlChange(0, keyMidiCcMapping[i].cc, 0);
            SerialDebug.printf("cc: %d off\n", keyMidiCcMapping[i].cc);
        }
    }
}

#if 0 /* just for testing without synthesizer */
void Midi_NoteOn(uint8_t ch, uint8_t note, uint8_t vel)
{
    SerialDebug.printf("on: %d\n", note);
}

void Midi_NoteOff(uint8_t ch, uint8_t note)
{
    SerialDebug.printf("off: %d\n", note);
}
#endif

void printKey()
{
    // getOemKey() returns the OEM-code associated with the key
    SerialDebug.print(" key:");
    SerialDebug.print(keyboard.getOemKey());

    // getModifiers() returns a bits field with the modifiers-keys
    int mod = keyboard.getModifiers();
    SerialDebug.print(" mod:");
    SerialDebug.print(mod);

    SerialDebug.print(" => ");

    if (mod & LeftCtrl)
    {
        SerialDebug.print("L-Ctrl ");
    }
    if (mod & LeftShift)
    {
        SerialDebug.print("L-Shift ");
    }
    if (mod & Alt)
    {
        SerialDebug.print("Alt ");
    }
    if (mod & LeftCmd)
    {
        SerialDebug.print("L-Cmd ");
    }
    if (mod & RightCtrl)
    {
        SerialDebug.print("R-Ctrl ");
    }
    if (mod & RightShift)
    {
        SerialDebug.print("R-Shift ");
    }
    if (mod & AltGr)
    {
        SerialDebug.print("AltGr ");
    }
    if (mod & RightCmd)
    {
        SerialDebug.print("R-Cmd ");
    }

    // getKey() returns the ASCII translation of OEM key
    // combined with modifiers.
    SerialDebug.write(keyboard.getKey());
    SerialDebug.println();
}

#endif
