/***********************************************************************
* {{ CO2 IKEA CLOCK }}
* Copyright (C) {{ 2022 }}  {{ The Meerkat Group }}
* 
* FILENAME :  functions.h
*
* DESCRIPTION : 
*   This is the functions include file for the IKEA clock CO2 meter. 
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
*********************************************************************************/




/*Function *************************************************************
 * Name: updateBrightness()
 * purpose  Sets a brightness level depending on the value of ambient light as red by the LDR.
 * Inputs
 * Outputs
 * Uses
 * Key values for the LDR are: Analog voltage >3 V is dark.
 * <1 V is licht.
 * the maximum brightness we want to use is 200. 
 * The minimum brightness is set to 10
 * Both values are defined in the declarations file.
 * We can linear divide 200 over the range 0 - 3 V.
 * This comes down to the following calculation:
 * Brightness = (3 - Vin) * (Max - Min)
 * 3Volt corresponds to appr. 600 in the ADC.
  * Values are experimental
  * Caution: brightness is a byte, ldrvalue a word.
 * 
 */

inline void updateBrightness()
{
   unsigned int ldrValue = analogRead(INPUT_LDR);
   if (ldrValue > 400) 
     {
      strip.setBrightness(0);
     }
   else
   {
     if (ldrValue > 100) ldrValue=80;
     strip.setBrightness(100- ldrValue);
   }
}
/***********************************************************************/

/*Function *************************************************************
 * Name:    setErrorCode
 * purpose  sets an Errorcode on the errorcode leds in Ring 1
 *          The led in RING 5 is set to red. The whole sthing is overwritten 
 *          later by the clock uodater
 * Inputs   Errorcode
 * Outputs  none
 * Uses     strip
 */
void setErrorCode(byte errorCode)
  {
    // Ring2 used for Errorcodes are dsiplayed on this ring
    for (byte i = 0; i< 16; i++) 
      {
        if(i < errorCode) strip.setPixelColor(RING2 + i, COLOUR_BLUE);
        else strip.setPixelColor(RING2 + i, 0);
      }
    strip.setPixelColor(RING5, COLOUR_RED); //Set led 61 to red to indicate a problem
    strip.show();
  }
/***********************************************************************/


/*Function *************************************************************
 * Name:    startTimer
 * purpose  starts a software timer
 * Inputs   timer number
 * Outputs  none
 * Uses     g_timers[]
 */
inline void startTimer(byte timerID)
{
    g_timers[timerID].Start = false;
    g_timers[timerID].Over  = false;
    g_timers[timerID].Count = g_timers[timerID].InitialValue;
    g_timers[timerID].Start = true;
}
/***********************************************************************/


/*Function *************************************************************
 * Name:    checkDoor
 * purpose  checks to see if the door is open. If the door is open, everything stops!
 *          as long as teh door is open, the program loops here.
 * Inputs   none
 * Outputs  none
 * Uses     nothing
 */
inline void checkDoor(void)
{
  if(digitalRead(INPUT_DOOR) == HIGH)
    {
      strip.clear();
      setErrorCode(ERROR_DOOR_OPEN);               // show event door open
      while (digitalRead(INPUT_DOOR)==HIGH)
      {
        //delay
        delay(500);
      }
  setErrorCode(EVENT_DOOR_CLOSE);                  // show event door closed
    }    
}
/***********************************************************************/



/*Function *************************************************************
 * Name:  updateClock
 * purpose  updates the clock structure every 15 seconds. Then updates the ring
 *   
 * Led counts start at 0 for the led at the '12 o clock position)
 * The ring has then 24/16/12/8/1 Led.
 *   Ring 24 is used to display the hours
 *   Ring 16 is used to display minutes in chunkcs of 5
 *   Ring 8 is used to display minutes in the 5 intervals.
 *   So: 10:24 is displayed as
 *   Ring 24 Led 10
 *   Ring 16 Led:  24/5 = 4 
 *   Ring 8 Led:     24 - (20*5) = 4 > leds 7 and 8
 *  The ringColour is defined by the CO2 level   
 *   the leds in the 16 ring are not used for the time. 
 *   They are used to indicate error conditions.
 *
 * Inputs   none
 * Outputs  none
 * Uses     g_rtc, g_timers[2], g_showDisplay, g_ringColour
 * Updates  g_localTime[] 
 *          strip
 */
inline void updateClock()
{
 if(g_timers[2].Over)
   {    
     // Only update the clock every (Timer 2) seconds
     DateTime now        =  g_rtc.now();      // read the time
     g_localTime.hour    = now.hour();
     g_localTime.minute  = now.minute();
     g_localTime.year    = now.year();
     g_localTime.month   = now.month();
     g_localTime.day     = now.day();
    // local kept time structure is updated
    byte minutesMod =  g_localTime.minute/5; // we need that a few times later on
     //update the rings
     if(g_showDisplay)
     {
      //LED 0 is always on.
      strip.setPixelColor(0,g_ringColour); // Led 0 is always on
      strip.setPixelColor(RING5,0);      // Clear led 61 (it could have turned on because of an error)

      for (byte i=0; i<16; i++)
      {
        strip.setPixelColor(i + RING2,0); // Clear errorcode on Ring2
      }

      // set the outer ring, hours
      for (byte i = 0 ; i<24 ; i++)
        {
        // set the hours on the first 24 leds
        // Do so in 12 hour system, will give 2 leds per hour. 
        byte twelveHour = g_localTime.hour;
        if  ( twelveHour>12) twelveHour = twelveHour-12;
        if (i<= 2*twelveHour) strip.setPixelColor(i,g_ringColour);  
        else strip.setPixelColor(i,0);
        }

      // Set the 12 led ring (Ring 3)
      for (byte i=0; i< 12 ; i++)
        {  
         //Set the 5 minute bloks in the 12 ring, starting at led 40
         if (i< minutesMod+1) strip.setPixelColor(i+RING3,g_ringColour);
         else strip.setPixelColor(i+RING3,0)   ; 
        }

      // Set the minute ring (ring 4)
      for (byte i=0; i< 8; i++)
        {
         byte minutesAdd =  2* (g_localTime.minute - (5* minutesMod));
         // Set the minute blocks in the 8 ring, starting at 52
         if (minutesAdd > i ) strip.setPixelColor(i+RING4,g_ringColour);
         else strip.setPixelColor(i+RING4,0)   ; 
         }  
      strip.show();   // Update the display
      }
     startTimer(2);  // Restart the Timer when done
    }
}
/***********************************************************************/


/*Function *************************************************************
 * Name:    setColorLevel
 * purpose: converts teh co2level to a color
 * Inputs
 * Outputs
 * Uses
 * 
 */
inline void setColorLevel(int actualCo2Level)
  {
    byte green, red,blue;
    if (actualCo2Level <256)
        {
          // In this are the colour chnages from blue to green.
          blue  = 255-actualCo2Level;
          green = actualCo2Level;
          red   = 0;
        }
    else if(actualCo2Level< 1024 )
        {
          // in this range, colour chnages from green to yellow
          blue  = 0;
          green = 255;
          red   = actualCo2Level/4;
        }
    else if ( actualCo2Level< 2048)
        {
          blue  = 0;
          green = 128- (actualCo2Level/16) ; 
          red   = 255;
          g_showDisplay = true;     // if the CO2 level gets high, override the display of setting.
        }
    else 
       {
          blue  = 0;
          green = 0;
          red   = 255;    // all red
          g_showDisplay = true;     // if the CO2 level gets high, override the display of setting.
       }
    g_ringColour = strip.Color(red, green, blue);   // Set the ring colour based on the CO2 level
  }
/***********************************************************************/


/*Function *************************************************************
 * Name: Read CO2 value
 * purpose  
 * Inputs 
 * Outputs 
 * Uses
 * This function is called in the main loop. 
 */
inline void getCO2 ()
{
  byte Co2RxBuf[CO2_BUFFER_SIZE];
  bool co2LevelReceived = false;
  if(g_timers[1].Over==true)
    {
      startTimer(1);                          // Restart the timer
      Serial.write(INIT_CO2, INIT_CO2_LENGTH) ;  // Send the Co2 command
      startTimer(0);                          // this is a time out for waiting for a reply      
      while (g_timers[0].Over==false && co2LevelReceived==false)
        {
        // As long as the timer is running
        if (Serial.available() ==9)     //the co2 sensor sends back 9 bytes
          {
          // received the string from the CO2 sensor
          Serial.readBytes(Co2RxBuf, 9);
          co2LevelReceived=true;
          g_co2Level = Co2RxBuf[2]*256 + Co2RxBuf[3] ;        // value of the CO2 mesurement in ppm
          g_timers[0].Start = false;                          // Stop the timer looking after the time-out
          setColorLevel(g_co2Level);
          }
        }
      if(g_timers[0].Over==true)  
      { 
         // a time out occured  
        setErrorCode(ERROR_TIMEOUT_CO2);      // set pixel 61 to red and error message 7
        g_co2Level    = 0;
      }
    }
}  
/***********************************************************************/



/*Function *************************************************************
 * Name:    receiveIR
 * purpose: receive a command from the IR inetrface
 * Inputs
 * Outputs: returns teh transaletd values of the pressed key
 * Uses
 * 
 */

/***********************************************************************/
inline byte receiveIR()
  {
   byte receivedIR;
   byte returnCmd= NO_CMD;         // default to no command, in case no IR signal is received.
   if (IrReceiver.decode())
      {
         receivedIR=IrReceiver.decodedIRData.command; 
          if (!(IrReceiver.decodedIRData.flags & (IRDATA_FLAGS_IS_AUTO_REPEAT | IRDATA_FLAGS_IS_REPEAT))) 
            {
              // This happens only if a key is NOT repeated
              g_countOK=0;
              switch(receivedIR)
                {
                 case 0x45: returnCmd  = 1;         break;
                 case 0x46: returnCmd  = 2;         break;
                 case 0x47: returnCmd  = 3;         break;
                 case 0x44: returnCmd  = 4;         break;
                 case 0x40: returnCmd  = 5;         break;
                 case 0x43: returnCmd  = 6;         break;
                 case 0x7:  returnCmd  = 7;         break;
                 case 0x15: returnCmd  = 8;         break;
                 case 0x9:  returnCmd  = 9;         break;
                 case 0x16: returnCmd  = KEY_AST;   break;
                 case 0x19: returnCmd  = 0;         break;
                 case 0x0D: returnCmd  = KEY_HASH;  break;
                 case 0x18: returnCmd  = KEY_UP;    break;
                 case 0x8:  returnCmd  = KEY_LEFT;  break;
                 case 0x1C: returnCmd  = KEY_OK;    break;
                 case 0x5A: returnCmd  = KEY_RIGHT; break;
                 case 0x52: returnCmd  = KEY_DOWN;  break;
               }
            }
          else
            {
            // You get here when a key is repeated
            // We process this only as long as we are in RUN mode
            if(g_runMode==RUN && receivedIR== 0x1C) //command decoder has not run yet, so need the hexcode.
              {
               returnCmd=NO_CMD;
               g_countOK++;
               if (g_countOK==10)
                {
                   // Start the timeout on the command mode
                  startTimer(3);
                  g_runMode=CMD;
                  strip.clear();
                  strip.setPixelColor(RING5,COLOUR_ORANGE);  
                  strip.show();
                  g_digitCount=0; // reset the digit count
                }
             }
           }
      IrReceiver.resume();
      }
  return(returnCmd);
  }
/***********************************************************************/


/*Function *************************************************************
 * Name:    showEntry();
 * purpose: shows the entered digit on the display.
 *          Ring 3 shows the entered value
 *          Rimg 2 shows the position (you need 10 entries: ddmmyyhhmm)
 * Inputs
 * Outputs
 * Uses
 */
void showEntry(byte entryCode, byte position)
  {
   strip.clear();
   if(entryCode>9)
    {
      // This is an error. do not show the entry dot, and show the value in red
      for (byte i = 0; i<position; i++) {strip.setPixelColor(i+RING3,COLOUR_RED); }
    }
   else
    {
      // This is normal mode, SHow the value entered and the positon dot  
      for (byte i = 0; i< entryCode; i++){strip.setPixelColor(i+RING3,COLOUR_ORANGE); } 
      for (byte i = 0; i<position;   i++){strip.setPixelColor(i+RING2,COLOUR_ORANGE); }
    }
   strip.show();
  }

/***********************************************************************/



/*Function *************************************************************
 * Name:    runTimeCommandProcessing();
 * purpose: shows the entered digit on the display.
 *          Ring 3 shows the entered value
 *          Rimg 2 shows the position (you need 10 entries: ddmmyyhhmm)
 * Inputs
 * Outputs
 * Uses
 */
inline void runTimeCommandProcessing(byte rxcmd)
{
byte digits;
byte displayDigit;
byte offset;
int  co2Display;           // these four are used to dispaly the CO2 level on the IR remote control key

 switch(rxcmd)
          {
          case KEY_AST: { 
                        /* The "*" switches the display off */
                        g_showDisplay= false;
                        strip.clear();
                        strip.show();     // otherwise nothng happens, not even clear.
                        break;
                        }
          case KEY_HASH: {
                        /* the "#" switches teh display on  */
                        g_showDisplay= true; 
                        g_timers[2].Start=false;
                        g_timers[2].Over=true;    // force an updat eof the clock display. 
                        break;
                        }
          case KEY_UP:   {
                        //Display date. Ring 0 & 1: day of the month
                        //Ring 2: Month This will clear when the display is updated again. To make sure you have a reasonable
                        // time, the timer is started again. (Timer 2)
                        startTimer(2);
                        strip.clear();
                        for (byte i=0; i<31 ; i++)
                          {   
                          if (i<g_localTime.day) strip.setPixelColor(i,g_ringColour);  
                          else strip.setPixelColor(i,0);
                          }
                        for (byte i=0; i<12; i++)
                         {
                          if (i<g_localTime.month) strip.setPixelColor(i+RING3,g_ringColour);  
                          else strip.setPixelColor(i+RING3,0);
                         } 
                        strip.show();
                        break;
                        }  
          case KEY_LEFT: {
                         // Display the real CO2 level on the rings. Ring 4 is MSD!
                         startTimer(2);
                         strip.clear();
                         digits =4;
                         co2Display = g_co2Level;
                         while (co2Display>0)
                          {
                           displayDigit=co2Display%10 + 1; 
                           co2Display = co2Display/10;
                           switch(digits)
                           {
                             case 4:  offset = 52; break;
                             case 3:  offset = 40; break;
                             case 2:  offset = 24; break;
                             case 1:  offset =  0; break;
                             case 0:  offset =  0; break;
                           }
                           for( byte i=0; i< displayDigit; i++){strip.setPixelColor(i+ offset, g_ringColour);}
                           digits--;
                          }
                         strip.show();
                         break;
                        }                   
          }// End switch
    }
/***********************************************************************/

/*Function *************************************************************
 * Name:    runTimeCommandProcessing();
 * purpose:  command handler:
 * you must enter the whole time date string like this: ddmmyyHHmm. 
 * The number you enter is alwasy shown in the second ring 3 (which has 12 leds. 0 is signified by teh LED at '12'
 * Clear the display
 * Stop the clock update timer for the time being
 * Start a time out
 * Inputs
 * Outputs
 * Uses
 */
   
inline void  cmdTimeCommandProcessing(byte rxcmd)
{
  if(rxcmd < KEY_UP)
        {
        // Handle the timesetting sequence
         startTimer(3);                   // for every key, restart the timeout.  
         g_timers[2].Start = false;         // stop the clock update timer            
         showEntry(rxcmd,g_digitCount);
         switch (g_digitCount)
            {
            case 0: { // MSD day
                     if(rxcmd>3) { showEntry(255,g_digitCount );/* this is an error */}
                     else {
                          g_newDay = rxcmd;
                          g_digitCount++;
                          }
                     break;
                    }
            case 1: { // LSD day
                    g_newDay = g_newDay*10+ rxcmd;
                    g_digitCount++;
                    break;
                    }
            case 2: { // MSD Month
                     if(rxcmd>1) { showEntry(255,g_digitCount);/* this is an error */}
                     else {
                          g_newMonth = rxcmd;
                          g_digitCount++;
                          }
                      break;
                    }
            case 3: { // LSD Month
                     g_newMonth = g_newMonth*10 + rxcmd;
                     g_digitCount++;
                    break;
                    }

            case 4: { // MSD year
                     g_newYear = rxcmd;
                     g_digitCount++;
                     break;
                    }
            case 5: { // LSD Year
                     g_newYear = 2000 + (g_newYear*10 + rxcmd);
                     g_digitCount++;
                     break;
                    }       
            case 6: { // MSD hour
                    if(rxcmd>2) { showEntry(255,g_digitCount);/* this is an error */}
                    else {
                          g_newHour = rxcmd;
                          g_digitCount++;
                         }
                    break;
                    }
            case 7: { // LSD Hour. Error checking is bit more complex here
                     if((g_newHour*10 + rxcmd)>23) { showEntry(255,g_digitCount);/* this is an error */}
                     else {
                          g_newHour = g_newHour*10 + rxcmd;
                          g_digitCount++;
                          }
                      break;
                    }
            case 8: { // MSD Minute
                     if(rxcmd>5) { showEntry(255,g_digitCount);/* this is an error */}
                     else {
                          g_newMinute = rxcmd;
                          g_digitCount++;
                          }
                     break;
                    }
            case 9: { // LSD Minute
                     g_newMinute= g_newMinute*10 + rxcmd;
                     g_digitCount++;
                     break;
                    }
            default:{
                    // This happens if you enter too may digits.
                    g_runMode= RUN;
                    g_timers[3].Start = false;     // stop the timeout.
                    startTimer(2);               // switch on the update timer again
                    }
                
            } // End switch
          }    // end if(rxcmd < keyUp)

  if(rxcmd == KEY_OK)
        {
         if (g_digitCount ==10)
            {
              // Time complete received
              //int newTime = 100 * newHour+ newMinute;
              //Serial.print(newDay,DEC); Serial.print("-");Serial.print(newMonth,DEC);Serial.print("-");Serial.println(newYear,DEC);
              //Serial.println(newTime,DEC);
              // Update the RTC with the new value
              g_rtc.adjust(DateTime(g_newYear, g_newMonth, g_newDay, g_newHour, g_newMinute, 0));
            }
           /* Also if you did not receive all keys, return to normal mode again */
           g_runMode= RUN;
           g_timers[3].Start = false;     // stop the timeout
           g_timers[2].Start = false;     // stop the clock update
           g_timers[2].Over  = true;      // set teh timeput to provoke an update now.
         } // End Commnd OK
      }
/***********************************************************************/


/*Function *************************************************************
 * Name:    IRcommandHandler();
 * purpose: 
 * Inputs
 * Outputs
 * Uses
 * The command is :
 * 
 *
 ***********************************************************************/

inline void IRcommandHandler()
{
  if (g_timers[3].Over)
     {
       /* This is a command timeout
        * return to RUN mode , clear the display and show the display again.
        */
       strip.clear();
       g_showDisplay=true;
       g_runMode=RUN;
       g_timers[3].Over=false;    // Reset the time out flag
       startTimer(2);           // Restart the clock update timer;
     }
  else
    {
    g_command=receiveIR();
    //There is a set of keys processed in RUN mode
    if (g_command != NO_CMD)
      {
       if (g_runMode==RUN ) 
        {
          runTimeCommandProcessing(g_command);
          g_command= NO_CMD;   // Reset the command after processing
        }  
       if (g_runMode==CMD ) 
        {
          cmdTimeCommandProcessing(g_command);
          g_command= NO_CMD;   // Reset the command after processing
        }
      }
    }
}