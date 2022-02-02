/***********************************************************************
* {{ CO2 IKEA CLOCK }}
* Copyright (C) {{ 2022 }}  {{ The Meerkat Group }}
* 
* FILENAME :  declarations.h
*
* DESCRIPTION : 
*   This is the declarations include file for the IKEA clock CO2 meter. 
*   It follows C coding guidelines, but does not refer to a *.c file
*   to increase ease of use (no need to compile and link this file)
*   Guidelines followed:
*   Global variables are preceded by "g_"
*   Constants are in uppercase with underscore as separator
*   Constants are defined as "const" wherever possible to allow the compiler to do type checking.
*
* NOTES : 
*
* AUTHOR :    Petermaria van Herpen        START DATE :    1-JAN-2022
* 
* LICENSE:
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
*
* CHANGES : 
* REF NO  VERSION DATE    WHO     DETAIL
*
*
*********************************************************************************
 * Ports      
 * SDARTC   A4 (default)
 * SCLKRTC  A5 (default)
 * IRReceive D7
 * Ledring   D6
 * CO2 init input   D3
 * Doorswitch   D2
 * D1/Do, CO2 Rx/Tx
 * LDR  A0
 * An open door will stop logging.
 *                                                                 *
 ********************************************************************************/
#define INPUT_LDR A0
const byte INPUT_DOOR     = 2;
const byte OUTPUT_CO2INIT = 3;



/********************************************************************************/
/* General Constants                                                            */
/********************************************************************************/
const byte RUN = 1;
const byte CMD = 2;
const byte ERROR_DOOR_OPEN   = 1;
const byte ERROR_TIMEOUT_CO2 = 2;
const byte EVENT_DOOR_CLOSE  = 3; 

/********************************************************************************
 * Software timer values                                                        *
 * Timer usage                                                                  *
 * Timer 0  Time out for the sensor                                             *
 * Timer 1  Read the CO2 level                                                  *
 * Timer 2  Read the time from the RTC 
 * Timer 3  Command time out                              
 *  The software timers use hardware timer 1. 
 * The tick is ste to 500 ms, as this is more than detailed enough for the
 *  tasks at hand and will decrease codesize and power.
 * Setup: prescale 256
 * delivers 62.5kHz to the timer. For an interrupt of 500ms, this then needs a 
 * value of 31250, well within the 16 bits of the timer
 * The counter counts up and interrupts on overflow, so we need to load the 
 * value 2ˆ16-31250 = 34286
 * The software timers are all 8 bits, giving a maximum  time of 128 seconds. 
 *
 * *    Hardware time registers
 * TCCR1A
 *        |** 7 **|** 6 **|** 5 **|** 4 **|** 3 **|** 2 **| ** 1 **|** 0 **|
 *        |COM1A1 |COM1A0 | COM1B1| COM1B0|   0   |   0   |WGM11   |WGM10  |
 *            0      0         0       0      0       0        0       0
 * No compare oututs, normal mode of operation
 * 
 * TCCR1B
 *        |** 7 **|** 6 **|** 5 **|** 4 **|** 3 **|** 2 **| ** 1 **|** 0 **|
 *        | ICNC1 |ICNC0  |   0   | WGM13 | WGM12 | CS12  |  CS11  | CS10  |
 *            0      0        0       0      0       1        0       0
 *         No noise cancel          normal mode      clock select f/256
 *
 * TIMSK1
 *        |** 7 **|** 6 **|** 5 **|** 4 **|** 3 **|** 2 **|** 1 **|** 0 **|
 *        |   0   |   0   | ICIE1 |   0   |   0   | OCIE1B|OCIE1A |TOIE1  |
 *            0      0         0       0      0       0        0       1
 * Overflow interrupt enabled
 *
 *
 ********************************************************************************/
const unsigned int T1_COUNT = 34286;
const byte TCCR1B_INIT = 4;

const byte NUMBER_OF_TIMERS = 4;
const unsigned int TICK = 500;   //Tick is 500 ms
typedef struct 
    {
    bool Start;
    byte Count;
    bool Over;
    byte InitialValue;
    } Timer;
Timer g_timers[NUMBER_OF_TIMERS];

const byte  Timer0Value =  2000 /TICK ; //Timer 0, 2 second timeout on the Co2 sensor
const byte  Timer1Value =  5000 /TICK;  //Timer 1 used to read the CO2 level, every 60 seconds One minute value is 120
const byte  Timer2Value = 15000 /TICK;  //Timer 2 used for a to update the clock from the RTC
const byte  Timer3Value =  6000 /TICK;  //Timer 3 used for Command time out. After this time, mode returns to "RUN" 

/********************************************************************************
 * Neopixel parameters                                                          *
 * 
 * Parameter 1 = number of pixels in strip
 * Parameter 2 = Arduino pin number (most are valid)
 * Parameter 3 = pixel type flags, add together as needed:
 * NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
 * NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
 * NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
 * NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
 * NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
 * IMPORTANT: To reduce NeoPixel burnout risk, add 1000 uF capacitor across
 *  pixel power leads, add 300 - 500 Ohm resistor on first pixel's data input
 *  and minimize distance between Arduino and first pixel.  Avoid connecting
 *  on a live circuit...if you must, connect GND first.
 ********************************************************************************/
#include <Adafruit_NeoPixel.h>
#define PIN 6

Adafruit_NeoPixel strip = Adafruit_NeoPixel(61, PIN, NEO_GRB + NEO_KHZ800);
uint32_t g_ringColour;

const unsigned long COLOUR_RED    = 0x0FF0000;
const unsigned long COLOUR_GREEN  = 0x00FF00 ;
const unsigned long COLOUR_BLUE   = 0x0000FF ;
const unsigned long COLOUR_ORANGE = 0x00FF00 ;       //re-defined as green.

bool g_showDisplay;    // If false, display will not be shown. (used in clock and runtime command handler)
const byte RING1 =  0;
const byte RING2 = 24;
const byte RING3 = 40;
const byte RING4 = 52;
const byte RING5 = 60;


/********************************************************************************/
/* RTC parameters and libraries                                                 */
/********************************************************************************/
#include <RTClib.h>
RTC_DS1307 g_rtc;

// structure to hold the localtime. 
typedef struct 
{
    byte hour;
    byte minute;
    int year;
    byte month;
    byte day;
} localTimeStruct;

localTimeStruct g_localTime;

/********************************************************************************/
/* Infrared control parameters and libraries                                    */
/********************************************************************************/
#include <IRremote.h>
#include <Wire.h>
const byte IR_RECEIVE_PIN = 7;
byte g_cmdReceived;
byte g_countOK;      // number of repitions before mode changes
byte g_runMode;
byte g_command;
byte g_digitCount; // Counts digits in timesetting. We need ddmmyyhhmm = 10 digits
int timeSet;
const byte KEY_UP    = 10;
const byte KEY_DOWN  = 11;
const byte KEY_LEFT  = 12;
const byte KEY_RIGHT = 13;
const byte KEY_OK    = 14;
const byte KEY_AST   = 15;
const byte KEY_HASH  = 16;
const byte NO_CMD   = 100;
const byte OFF      = 1;
const byte ON       = 2;

// These store the values from the IR interface.
// These are globals as they need to be preserved in between calls to the IR function.
byte  g_newDay;
byte  g_newMonth;
byte  g_newHour;
byte  g_newMinute;
int   g_newYear;



/********************************************************************************
 * CO2 sensor                                                                   *
 ********************************************************************************/

const byte INIT_CO2[]     = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
const byte INIT_CO2_LENGTH = 9; 
unsigned int g_co2Level;    // value of the CO2 mesurement in ppm
const byte  CO2_BUFFER_SIZE = 15; 


/********************************************************************************
 * End of delcations                                                            *
 ********************************************************************************/
