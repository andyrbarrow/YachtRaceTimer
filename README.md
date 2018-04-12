# YachtRaceTimer
A countdown timer for ESP32 using the Arduino IDE

This is a countdown timer for ESP32, specifically the TTGO ESP development board which includes an OLED display and has a built-in battery and charging circuit. The board is available from Aliexpress here https://www.aliexpress.com/item/TTGO-WiFi-Bluetooth-Battery-ESP32-0-96-inch-OLED-development-tool/32827899155.html. I'm sure it is available elsewhere. This particular timer has been done for a number of different Arduino-compatible platforms, and has gone through a few iterations in display, from LEDs, to a 4 digit display, to now an OLED display. Using the development board reduced the hardware count substantially, as batteries, charging, and display are integrated on the board. It should be easy to port to another ESP or Arduino platform.

Hardware needed for this project are:
- TTGO WiFi & Bluetooth Battery ESP32 Rev1 0.96 inch OLED development tool
- An Arduino-compatible relay module
- Two push buttons
- A small 5v audio amplifier (I used the DROK Super Small 3W+3W DC 5V Audio Amplifier available on Amazon)
- A small speaker
- An enclosure, preferably small enough to be hand-held.
- Hookup wire, connectors, etc.

It is assumed that this system would replace a single pushbutton normally used with sailboat race electric horns, typically those created from automobile or truck horns with a horn relay. I suggest using an additional connector in parallel with the original pushbutton so that both can be used to activate the horn. This provides a backup, and a means of activating the horn in situations that the timer wasn't designed for, such as OCS, General Recall, etc. 

This system is specifically designed to accomodate three countdown modes, consistent with normal sailboat race operations:
- 6 minute timer (Horns for AP Down, Class Flag Up, Preperatory Flag Up, Preparatory Flag Down, and Class Flag Down/Start)
- 5 minute timer (Horns for Class Flag Up, Preparatory Flag Up, Preparatory Flag Down, and Class Flag Down/Start)
- 5 minute repeating (Same as above, but the sequence continues until the timer is reset)
On the first two modes, the timer will count up after the start. This is particularly useful if you have an OCS and you need to count 3 minutes after the OCS flag, or if you want to know the time to first mark, total time of the race, etc.

The timer uses the amplifier and speaker to alert the deck crew of an upcoming flag change. At 50, 40, 30, and 20 seconds to the flag change, a long beep is sounded in the speaker. At 15-11 seconds, a single short beep is heard, 10-6 two short beeps and at 5-1 three short beeps are heard. At the flag, the relay is activated to sound the horn. The next flag requirement is displayed on the OLED, as is the countdown to the flag.

This sketch requires Nick Gammon's Switch Manager, which can be downloaded from:
http://gammon.com.au/Arduino/SwitchManager.zip. Install this in your Arduino IDE using Library Manager/Install from Zip
