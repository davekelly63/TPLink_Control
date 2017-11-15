# TPLink_Control
Using NodeMCU to control TPLink smart bulb

Written using Arduino, and ESP8266 libraries

Requires NodeMCU Arduino library from http://arduino.esp8266.com/stable/package_esp8266com_index.json

### Background
The TPLink bulb is controllable by phone app or Alexa, but in this case the location was inconvenient to use either of those methods.

So this project was created to use the NodeMCU board, and fit inside a large pushbutton switch. The light can then be switched on or off remotely.

Unfortunately the connect time of the NodemMCU meant it is impractical to just power the button when required, then go back to sleep. This would be ideal and allow it to be run on batteries, but the lag for the connection is between 200ms and 5 seconds, which is unacceptable.

So the device must be continuously powered, monitoring for the push switch. It does mean that it is responsive and the light is switched within 300ms.

### Operation
WiFiManager is used to manage the connection. This library (from https://github.com/tzapu/WiFiManager) provides an AP when powered up for configuring the connection, but afterwards it will automatically connect to the stored router. If the wifi point is not available, it will automatically show the AP again.

The switch input is connected to GPIO.5, with a pullup resistor.

When the NodeMCU powers up, it checks this switch state, and if the switch is active the AP will be shown, otherwise the NodeMCU will connect to the stored wifi.

Additionally a LED is connected to GPIO.13, which flashes while the NodeMCU is in the AP mode.

To simplify operation, the bulb has a static address set in the router. Future enhancement will allow a scan of the network to locate bulbs attached.

When the switch is pressed, the command to get the light status is sent out.

The bulb replies in typically 200ms with its current state. The NodeMCU can then send a command to turn the light on or off correspondingly.

Acknowledgements

Most information was gained from project https://github.com/konsumer/tplink-lightbulb. Commands were tested, and sniffed using Wireshark to obtain the basic commands.

Some commands were simply guesses until it worked (status)

Currently only the On, Off and Status commands are implemented.

A possible future enhancement is to use the length of switch press as a brightness control, ramping up and down.


