#include <Arduino.h>
#include <SPI.h>
#include "declarations.h"
#include "functions.h"
/***********************************************************************
* {{ CO2 IKEA CLOCK }}
* Copyright (C) {{ 2022 }}  {{ The Meerkat Group }}
* 
* FILENAME :  main.cpp
*
* DESCRIPTION : 
*   This is the main file for the IKEA clock CO2 meter. 
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
**********************************************************************
 * Main Part                                                          *
 * the functioanlity is overseeable. Read the CO2 level, read the clock
 * and display the result. The system has an IR interface to process some
 * simple commands.
 */
void setup() 
{
  // Hardware inits
  pinMode(INPUT_DOOR, INPUT_PULLUP);
  pinMode(OUTPUT_CO2INIT , OUTPUT);
  //All other pins are set by their libraries.

  //Set the init output for the CO2 module to low
  digitalWrite(OUTPUT_CO2INIT , LOW);
  g_showDisplay = true;           // Display is on.
  Serial.begin(9600);

  g_rtc.begin();      // start the rtc
  
  if (! g_rtc.isrunning()) 
    {
    // When time needs to be re-set on a previously configured device, the
    // following line sets the RTC to the date & time this sketch was compiled
     g_rtc.adjust(DateTime( F(__DATE__), F(__TIME__) ));
     //Comment: I am not entirely convinced this works reliable. 
    }

  //Setup the software timers
  g_timers[0].InitialValue = Timer0Value;
  g_timers[1].InitialValue = Timer1Value;
  g_timers[2].InitialValue = Timer2Value;
  g_timers[3].InitialValue = Timer3Value;
  
  TCNT1 = T1_COUNT;           // for for interrupt of Tick ms.
	TCCR1A = 0x00;
	TCCR1B = TCCR1B_INIT;       // Timer mode 
	TIMSK1 = (1 << TOIE1) ;    // Enable timer1 overflow interrupt(TOIE1)

  // Initialise the strip 
  strip.begin();
  strip.setBrightness(10);     // Set to low brightness during start up
  strip.show();                // Initialize all pixels to 'off'
  g_ringColour = COLOUR_BLUE;  // Set initial colour to blue;

  // Set the IR receiver
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
  g_command=NO_CMD;

  // Startup the CO2 sensor, It needs 7 seconds of 'low'at the input
  digitalWrite(OUTPUT_CO2INIT , LOW); 

  //Fill the dots one after the other with blue during startup This sequence takes 6.1 seconds
  for(byte i=0; i < 61 ; i++) 
    {
    strip.setPixelColor(i, g_ringColour);
    strip.show();
    delay(100);
    }
  
  // Clear the dots, which will take another 6.1 seconds.
  for(byte i=0; i < 61 ; i++) 
    {
    strip.setPixelColor(i, 0);
    strip.show();
    delay(100);
    }

  // Clear the init output again, this will now enable the CO2 sensor
  digitalWrite(OUTPUT_CO2INIT , HIGH);

  // force an update of the clock display
  g_timers[2].Over=true;
  updateClock();

// Start the timers
  startTimer(0);  
  startTimer(1);    
  startTimer(2); 

	sei();         // enable interrupts
  g_runMode=RUN;   // Run mode
}




/*************************************************************************************** 
 * Main loop                                                                           *
 *                                                                                     *
 ***************************************************************************************/ 

void loop() 
{
  updateClock();        //update the internal clock strcuture every (Timer 2) seconds
  checkDoor();          // see if the door is open. That will stop all functions and turn the center led red.
  getCO2();             //Get a new value from the CO2 sensor
  updateBrightness();   //adapt the brightness of the ring to the ambient light value
  IRcommandHandler();   // IR Commandhandler
}

/*************************************************************************************** 
 * Timer Interrupt                                                                     *
 *                                                                                     *
 ***************************************************************************************/ 
ISR (TIMER1_OVF_vect)
{
    // Timer interrupt
    for(byte i=0;i<NUMBER_OF_TIMERS;i++)
       {
        if(g_timers[i].Start)
        {
         g_timers[i].Count--;
         if(g_timers[i].Count==0)
           {
            g_timers[i].Start=false;
            g_timers[i].Over=true;
           }
         } 
        }
     TCNT1 = T1_COUNT;            // reload the timer value
}
