/* Sailboat Race Timer for ESP32
   Activates a relay for an electric horn during a stardard ISAF 5 minute start sequence.
   Also providing audio and visual alerts to users regarding upcoming flag actions

   Will operate in 6 Min, 5 Min, or 5 Min Cycle mode.

   This code is in the Public Domain, and covered by the General Public License.

   Thanks to:
   Nick Gammon (Switchmanager)



   Andy Barrow
*/

#include <SwitchManager.h>  // Manage the switch debounce functions
#include <Wire.h>           // For the OLED display
#include "SSD1306.h"        // alias for `#include "SSD1306Wire.h"` (for OLED)

static const uint8_t TRUE = 1;
static const uint8_t FALSE = 0;

//Switch Manager Stuff =================================================
//object instantiations
SwitchManager myStartSwitch;
SwitchManager myModeSwitch;

unsigned long currentMillis;
unsigned long heartBeatMillis;
unsigned long heartFlashRate    = 500UL; // time the led will change state
unsigned long startShortPress   = 500UL; // Start Button Short Press (MS)
unsigned long startLongPress    = 3000UL;// Start Button Long Press (MS)
unsigned long modeShortPress    = 500UL; // Mode Button Short Press (MS)

const byte heartBeatLED         = 5;
const byte startSwitch          = 16; //start switch ESP32 pin
const byte modeSwitch           = 17; //mode switch ESP32 pin

int myCounter;

// Tone Function========================================================
//The ESP32 doesn't have a tone library, so we use PCM
int freq = 2000;
int channel = 0;
int resolution = 8;
const byte TONE_PIN = 12;             // Tone Pin - this will go to an amp

// Relay output=========================================================
const byte RELAY = 14;                //Relay for horn

// Initialize the OLED display using Wire library=======================
SSD1306  display(0x3c, 5, 4);

//Countdown Timer State Machine=========================================
// This is the state we are in during the sequence
// State is one of:
// 0: Ready (Holding, no action)
// 1: One Minute to Class Up
// 2: Class Flag Up
// 3: Prep Signal Up
// 4: One Minute to Start (Prep Down)
// 5: Start (Class Down)

int state = 0;

// other variables=====================================================
unsigned long remainingFlagTime = 0;    // Used for countdown beeps and flashes
unsigned long remainingGoTime = 0;      // For future counter
unsigned long startTime = 0;            // time we started counting down
unsigned long classUpTime = 0;          // time to class up
unsigned long prepUpTime = 0;           // time to prep up
unsigned long prepDownTime = 0;         // time to prep down
unsigned long goTime = 0;               // time to start
unsigned int remainingFlagDisp = 0;     // time to display
short int clockRunning = FALSE;         // is the timer running?
const int toneFreq = 1000;              // tone frequency to output
const int shortToneLength = 70;         // length of a short tone (MS)
const int longToneLength = 700;         // length of a long tone (MS)
const int toneDelay = 100;              // delay between tones (MS)
const int longHornDelay = 1200;         // time of a long horn
const int shortHornDelay = 600;         // time of a short horn
char timeString[9];                     // the time that will be displayed
int countSeconds;                       // the last seconds value, so we don't update constantly
String flagString;                      // the flag string to print

// matrix to hold beep times and a variable to hold the index===========
const unsigned long beepMatrix[2][19] = {
  { 50, 40, 30, 20, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 }, //Seconds in the countdown
  {  4,  4,  4,  4,  1,  1,  1,  1,  1,  2, 2, 2, 2, 2, 3, 3, 3, 3, 3 }  //Number of beeps (4 = one long beep)
};
int beepIndex = -1;                 // the matrix index

// Countdown Mode
short int countMode = 1;

void setup()
{
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }

  //gives a visual indication if the sketch is blocking
  pinMode(heartBeatLED, OUTPUT);

  myStartSwitch.begin (startSwitch, handleSwitchPresses);
  myModeSwitch.begin (modeSwitch, handleSwitchPresses);
  //the handleSwitchPresses() function is called when a switch changes state

  //initialize PCM to produce tone
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(TONE_PIN, channel);

  // Set GPIOs to correct states
  pinMode(RELAY, OUTPUT);               // Relay
  digitalWrite(RELAY, LOW);             // Make sure it starts out in a low state

  // Initialising the UI will init the display too.
  display.init();

  //display.flipScreenVertically();     // This will put the display on the bottom
  display.setFont(ArialMT_Plain_24);

} //                   E N D  O F  s e t u p ( )

//======================================================================

void loop()
{

  //leave this line of code at the top of loop()
  currentMillis = millis();

  //***************************
  //some code to see if the sketch is blocking
  if (CheckTime(heartBeatMillis, heartFlashRate, true))
  {
    //toggle the heartBeatLED
    digitalWrite(heartBeatLED, !digitalRead(heartBeatLED));
  }

  // clear the display
  display.clear();

  myStartSwitch.check  ();
  myModeSwitch.check ();

  switch (state) {
    case 0:
        flagString = "READY";
        display.setTextAlignment(TEXT_ALIGN_CENTER);
        display.drawString(64, 0, String(flagString));
        display.display();
        break;

    case 1:                                     // Button was pushed - One minute to start class up
        if (startTime == 0) {                   // just started counting down, initialize
          startTime = millis();
          // calculate times
          classUpTime = startTime + 60000ul;     // class flag up
          prepUpTime = startTime + 120000ul;     // prep flag up
          prepDownTime = startTime + 300000ul;   // prep flag down
          goTime = startTime + 360000ul;         // start signal (go)
          beepIndex = 0;                         // array index for the minute countdown
          StatePrint(state);
          shorthorn();                            // this horn indicates AP coming down
        } else if (classUpTime >= millis()) {     //waiting for the flag
          remainingFlagTime = classUpTime - millis();
          remainingGoTime = goTime - millis();
          flagString = "Class UP";
          printCount(remainingFlagTime);
          if (beepIndex < 20)
            beepIndex = minuteCountDown(remainingFlagTime, beepIndex);
        } else {
          state = 2;
          shorthorn();
          StatePrint(state);
          beepIndex = 0;
        }
        break;
    case 2:                                       // We are in the first minute
        if (startTime == 0) {                     // just started counting down, initialize
          startTime = millis();                   // We're doing this again because we might be beginning timing from this point for modes 2 and 3
          // calculate times
          classUpTime = startTime;                // class flag up (this is actually unnecessary)
          prepUpTime = startTime + 60000ul;       // prep flag up
          prepDownTime = startTime + 240000ul;    // prep flag down
          goTime = startTime + 300000ul;          // start signal (go)
          beepIndex = 0;                          // array index for the minute countdown
          StatePrint(state);
          shorthorn();
        } else if (prepUpTime >= millis()) {       // waiting for the flag
          remainingFlagTime = prepUpTime - millis();
          flagString = "Prep UP";
          printCount(remainingFlagTime);
          remainingGoTime = goTime - millis();
          if (beepIndex < 20)
            beepIndex = minuteCountDown(remainingFlagTime, beepIndex);
        } else {
          // Prep Flag Up
          state = 3;
          shorthorn();
          StatePrint(state);
          beepIndex = 0;
          remainingFlagTime = 60000;
        }
        break;
    case 3:                                       //We are in prep
        if (prepDownTime >= millis()) {           //waiting for the flag
          remainingFlagTime = prepDownTime - millis();
          remainingGoTime = goTime - millis();
          flagString = "Prep DWN";
          printCount(remainingFlagTime);
          if (beepIndex < 20)
            beepIndex = minuteCountDown(remainingFlagTime, beepIndex);
        } else {
          // Prep Flag Down - enter final minute
          state = 4;
          longhorn();
          StatePrint(state);
          beepIndex = 0;
          remainingFlagTime = 60000;                //60 seconds to start
        }
        break;
    case 4:                                         //We are in last minute
        if (goTime >= millis()) {                   //waiting for the flag
          remainingFlagTime = goTime - millis();
          remainingGoTime = goTime - millis();
          flagString = "Class DWN";
          printCount(remainingFlagTime);
          if (beepIndex < 20)
            beepIndex = minuteCountDown(remainingFlagTime, beepIndex);
        } else {
          // Class Flag Down (start)
          if (countMode != 3)                      //we don't want to beep the horn twice when we start the next sequence
            shorthorn();
          state = 5;
          StatePrint(state);
          beepIndex = 0;
        }
        break;
    case 5:                                       //We have started
        unsigned long currentMillis = millis();
        // If we are in Mode 3, reset to state 2, set startTime to 0, then recycle
        if (countMode == 3) {
          state = 2;
          startTime = 0;
          return;
        } else {
          flagString = "STARTED";
          printCount(currentMillis - goTime);
        }
        break;
    //default:
      //break;
  }

} //                      E N D  O F  l o o p ( )


//======================================================================
//                          F U N C T I O N S
//======================================================================


//                        C h e c k T i m e ( )
//**********************************************************************
//Delay time expired function
//Parameters:
//lastMillis = time we started
//wait = delay in ms
//restart = do we start again

boolean CheckTime(unsigned long  & lastMillis, unsigned long wait, boolean restart)
{
  //has time expired for this task?
  if (currentMillis - lastMillis >= wait)
  {
    //should this start again?
    if (restart)
    {
      //yes, get ready for the next iteration
      lastMillis += wait;
    }
    return true;
  }
  return false;

} //                 E N D   o f   C h e c k T i m e ( )


//                h a n d l e S w i t c h P r e s s e s( )
//**********************************************************************

void handleSwitchPresses(const byte newState, const unsigned long interval, const byte whichPin)
{
  switch (whichPin)
  {
    //***************************
    //are we dealing with the start switch?
    case startSwitch:
      if (newState == HIGH)
      {
        //was this a short press followed by a switch release
        if (interval <= startShortPress)
        {
          Serial.println("Start button press");
          Serial.print("countMode = ");
          Serial.println(countMode);
          startTime = 0;
          switch (countMode) {
            case 1:
              state = 1;                        //Start in 6 minute mode (1 min delay to Class up)
              clockRunning = TRUE;
              break;
            case 2:
              state = 2;;                       //Start in 5 minute mode (immediate Class up)
              clockRunning = TRUE;
              break;
            case 3:
              state = 2;                        //Start in 5 minute cycle mode (immediate Class up)
              clockRunning = TRUE;
              break;
            default:
              state = 0;
              clockRunning = FALSE;
              return;
          }
        }
        //was this a long press followed by a switch release
        else if (interval >= startLongPress)
        {
          //Long press on the start button causes the system to reset
          Serial.println("Counter Reset");
          resetCounter();
        }
      }
      break; //End of case startSwitch

    //***************************
    //are we dealing with the mode switch?
    case modeSwitch:
      if (newState == HIGH)
      {
        //was this a short press followed by a switch release
        if (interval <= modeShortPress)
        {
          countMode++;
          Serial.println(countMode);
          if (countMode == 4) { //Increment the mode through three modes, and loop back to 1
            countMode = 1;
          }
          modeBeeps(countMode);
        }

      }

      else //the switch must have gone from HIGH to LOW
      {
        //if the switch is a normally closed (N.C.) and opens on a press this section would be used
      }

      break; //End of case modeSwitch

  } //End switch (whichPin)

} //      E n d   o f   h a n d l e S w i t c h P r e s s e s ( )


// Print what state we are in (for debugging)
void StatePrint(int state)
{
  Serial.print("Now in state: ");
  Serial.println(state);
}

// Short and Long Horn
void shorthorn()
{
  digitalWrite(RELAY, HIGH);
  delay(shortHornDelay);
  Serial.println("Short Horn");
  digitalWrite(RELAY, LOW);
}
void longhorn()
{
  digitalWrite(RELAY, HIGH);
  delay(longHornDelay);
  Serial.println("Long Horn");
  digitalWrite(RELAY, LOW);
}

// This function checks the countdown to see if we need to alert to a flag change
int minuteCountDown(unsigned long remainingTime, int beepMatrixIndex)
{
  unsigned long beepSeconds = beepMatrix[0][beepMatrixIndex]; //Get the countdown seconds we are looking for
  unsigned long beepCounter = beepMatrix[1][beepMatrixIndex]; //Get the number of beeps

  beepSeconds = beepSeconds * 1000ul;   //convert to milliseconds
  if (remainingTime >= beepSeconds)     //not there yet
    return beepMatrixIndex;             //return index given when called
  else {                                //beep
    beep(beepCounter);
    beepMatrixIndex++;                  //increment index
    return beepMatrixIndex;             //return incremented index
  }
}

// This function provides sounds during the last 50 seconds to any flag
void beep(int beepCount)
{
  switch (beepCount) {
    case 1:
    case 2:
    case 3:
      Serial.print(beepCount);
      Serial.println(" beeps");
      for (int i = 1; i <= beepCount; i++) {
        ledcWriteTone(channel, 1000);
        delay(shortToneLength);
        ledcWrite(channel, 0);
        delay(toneDelay);
      }
        break;
      case 4:
        Serial.println("Long Beep"); //Beep Count 4 is a long beep
        ledcWriteTone(channel, 1000);
        delay(longToneLength);
        ledcWrite(channel, 0);
        break;
      default:
        break;
  }
}
  // This resets all counters and state
  void resetCounter()
  {
    state = 0;
    remainingFlagTime = 0;
    remainingGoTime = 0;
    startTime = 0;
    classUpTime = 0;
    prepUpTime = 0;
    prepDownTime = 0;
    goTime = 0;
    remainingFlagDisp = 0;
    return;
  }

  // This gives an audible indication of the mode
  void modeBeeps(int modeBeepCount) {
    switch (modeBeepCount) {
      case 1: {
          Serial.println("Countmode = 1");        // Go around back to mode 1
          beep(1);
          //print_6min();                           // print "6'"
          break;
        }
      case 2: {
          Serial.println("Countmode = 2");        // switch to mode 2
          beep(2);
          //print_5min();                           // print "5'"
          break;
        }
      case 3: {
          Serial.println("Countmode = 3");        // switch to mode 3
          beep(3);
          //print_5minrep();                        // print "5'r
          break;
        }
      default:
        break;
    }
  }

  void printCount(unsigned long timeMillis)
  {
    unsigned long allSeconds = timeMillis / 1000;
    int runHours = allSeconds / 3600;
    int secsRemaining = allSeconds % 3600;
    int runMinutes = secsRemaining / 60;
    int runSeconds = secsRemaining % 60;

    if (countSeconds != runSeconds) {
      if (runHours < 1) {
        sprintf(timeString, "%02d:%02d", runMinutes, runSeconds);
      }
      else {
        sprintf(timeString, "%02d:%02d:%02d", runHours, runMinutes, runSeconds);
      }
      Serial.println(timeString);
      Serial.println(flagString);

      display.setTextAlignment(TEXT_ALIGN_CENTER);
      display.drawString(64, 0, String(flagString));
      display.drawString(64, 36, String(timeString));
      display.display();

      countSeconds = runSeconds;
    }

  }
  //======================================================================
  //                      E N D  O F  C O D E
  //======================================================================
