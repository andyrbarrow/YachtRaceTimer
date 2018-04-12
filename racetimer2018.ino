/* Sailboat Race Timer for Arduino
   Activates a relay for an electric horn during a stardard ISAF 5 minute start sequence.
   Also providing audio and visual alerts to users regarding upcoming flag actions

   Will operate in 6 Min, 5 Min, or 5 Min Cycle mode.

   This code is in the Public Domain, and covered by the General Public License.

   Andy Barrow
*/
#include <SwitchManager.h>             
SwitchManager myStartSwitch;
SwitchManager myModeSwitch;
static const uint8_t TRUE = 1;
static const uint8_t FALSE = 0;

// This is the state we are in during the sequence
// State is one of:
// 0: Ready (Holding, no action)
// 1: One Minute to Class Up
// 2: Class Flag Up
// 3: Prep Signal Up
// 4: One Minute to Start (Prep Down)
// 5: Start (Class Down)

int state = 0;

// other flag related variables
unsigned long remainingFlagTime = 0;   //Used for countdown beeps and flashes
unsigned long remainingGoTime = 0;     //For future counter
unsigned long startTime = 0;           // time we started counting down
unsigned long classUpTime = 0;         // time to class up
unsigned long prepUpTime = 0;          // time to prep up
unsigned long prepDownTime = 0;        // time to prep down
unsigned long goTime = 0;              // time to start
unsigned int remainingFlagDisp = 0;    // time to display
short int clockRunning = FALSE;        // is the timer running?

bool STARTED = 0; // the race has not started
int longHornTime = 1200; //Long Horn Time in MS
int shortHornTime = 600; //Short horn time in MS

// Relay and Tone output
const short int RELAY = 11;          //Relay

// Buttons

const byte startSwitch          = 17; //start switch is on Arduino pin 17
const byte modeSwitch          = 18; //mode switch is on Arduino pin 18

// delay before a cancel is allowed (ms)
unsigned long resetDelay = 3000UL;        // don't allow immediate cancellation, wait a few seconds
// time to press start button
unsigned long startDelay = 50UL;

// matrix to hold beep times and a variable to hold the index
const unsigned long beepMatrix[2][19] = {
  { 50, 40, 30, 20, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 }, //Seconds in the countdown
  {  4,  4,  4,  4,  1,  1,  1,  1,  1,  2, 2, 2, 2, 2, 3, 3, 3, 3, 3 }  //Number of beeps (4 = one long beep)
};
int beepIndex = -1;                 // the matrix index

int myCounter = 0;

//Tone Function
//The ESP32 doesn't have a tone library, so we use PCM
int freq = 2000;
int channel = 0;
int resolution = 8;
int tonePin = 12;

// Functions
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
  delay(shortHornTime);
  digitalWrite(RELAY, LOW);
}
void longhorn()
{
  digitalWrite(RELAY, HIGH);
  delay(longHornTime);
  digitalWrite(RELAY, LOW);
}

//**********************************************************************

void handleSwitchPresses(const byte newState, const unsigned long interval, const byte whichPin)
{
  //  You get here "ONLY" if there has been a change in a switches state.

  //When a switch has changed state, SwitchManager passes this function 3 arguments:
  //"newState" this will be HIGH or LOW. This is the state the switch is in now.
  //"interval" the number of milliseconds the switch stayed in the previous state
  //"whichPin" is the switch pin that we are examining  

  switch (whichPin)
  {
    //***************************
    //are we dealing with this switch?
  case startSwitch: 

    //has this switch gone from LOW to HIGH (gone from pressed to not pressed)
    //this happens with normally open switches wired as mentioned at the top of this sketch
    if (newState == HIGH)
    {
      //The incSwitch was just released
      //was this a short press followed by a switch release
      if(interval <= startDelay) 
      {
        Serial.print("My counter value is = ");
        myCounter++;
        if(myCounter > 1000)
        {
          //limit the counter to a maximum of 1000
          myCounter = 1000; 
        }
        Serial.println(myCounter);
      }

      //was this a long press followed by a switch release
      else if(interval >= resetDelay) 
        //we could also have an upper limit
        //if inc.LongMillis was 2000UL; we could then have a window between 2-3 seconds
        //else if(interval >= incLongMillis && interval <= incLongMillis + 1000UL) 
      {
        //this could be used to change states in a StateMachine
        //in this example however, we will just reset myCounter
        myCounter = 0;
        Serial.print("My counter value is = ");
        Serial.println(myCounter);
      }

    }

    //if the switch is a normally closed (N.C.) and opens on a press this section would be used
    //the switch must have gone from HIGH to LOW 
    else 
    {
      Serial.println("The startSwitch was just pushed");
    } 

    break; //End of case incSwitch

    //*************************** 
    //are we dealing with this switch?
  case modeSwitch: 

    //has this switch gone from LOW to HIGH (gone from pressed to not pressed)
    //this happens with normally open switches wired as mentioned at the top of this sketch
    if (newState == HIGH)
    {
      //The decSwitch was just released
      //was this a short press followed by a switch release
      if(interval <= decShortPress) 
      {
        Serial.print("My counter value is = ");
        myCounter--;
        if(myCounter < 0) 
        {
          //don't go below zero
          myCounter = 0;
        }
        Serial.println(myCounter);
      }

    }

    //if the switch is a normally closed (N.C.) and opens on a press this section would be used
    //the switch must have gone from HIGH to LOW
    else 
    {
      Serial.println("The decSwitch switch was just pushed");
    } 

    break; //End of case decSwitch

    //*************************** 
    //Put default stuff here
    //default:
    //break; //END of default

  } //End switch (whichPin)

} //      E n d   o f   h a n d l e S w i t c h P r e s s e s ( )

int myCounter;

void setup() {
  Serial.begin(115200);
  // Set up the buttons
  //instantiate buttons

  myStartSwitch.begin (startSwitch, handleSwitchPresses); 
  myModeSwitch.begin (modeSwitch, handleSwitchPresses);
    //initialize PCM to produce tone
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(tonePin, channel);
  
  //initialize the display
  //initialize the variables


}

void loop() {
 

}
