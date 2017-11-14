
#include <stdbool.h>
#include <stdint.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Ticker.h>
#include <WiFiManager.h>

const String AP_NAME = "TPLink Switch AP";

//const char * brightnessCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\": {\"transition_light_state\": {\"ignore_default\": 1, \"on_off\": 1, \"transition_period\": 0, \"brightness\": 25}}}"};
const char * onCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\": {\"transition_light_state\": {\"ignore_default\": 1, \"on_off\": 1, \"transition_period\": 0}}}"};
const char * offCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\": {\"transition_light_state\": {\"ignore_default\": 1, \"on_off\": 0, \"transition_period\": 0}}}"};
const char * detailsCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\":{\"get_light_details\":{}}}"};
const char * stateCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\":{\"get_light_state\":{}}}"};

WiFiUDP udp;

Ticker ticker;

bool lampState = false;
bool haveReply = false;
bool awaitingReply = false;

uint32_t counter = 0;

uint8_t timer = 0;

#define SW_INPUT    5
#define TEST_PIN    4
#define LED         13          // Connected to D7

void setup()
{
  pinMode (SW_INPUT, INPUT);                   // Switch input
  pinMode (TEST_PIN, OUTPUT);

  Serial.begin (115200);

  WiFiManager wifiManager;

  wifiManager.setAPCallback(ConfigModeCallback);

  if (digitalRead(SW_INPUT) == 0)
  {
    //first parameter is name of access point, second is the password
    wifiManager.autoConnect(AP_NAME.c_str()); // No password
  }
  else
  {
    // Switch is held down, show the Access Point page
    wifiManager.startConfigPortal(AP_NAME.c_str());
  }

  delay(100);

  PrintWifiStatus();

  Serial.println("udp Listen " + udp.begin(9999));

  Serial.println("Started");

  ticker.attach(0.5, tick);
}

void ConfigModeCallback(WiFiManager *myWiFiManager)
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());

  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void loop()
{
  // put your main code here, to run repeatedly:

  // if there's data available, read a packet
  uint16_t packetSize = udp.parsePacket();

  packetSize = udp.available ();

  if (packetSize > 0)
  {
    if (udp.remotePort() == 9999)
    {
      Serial.println ("Data available");
      // it is from the bulb

      char packetBuffer [250];
      uint16_t length = udp.read (packetBuffer, sizeof(packetBuffer));

      if (length > 0)
      {
        //Serial.print(length);
        //Serial.println (" bytes of data received from bulb");
        // Now decrypt it

        DecryptMessage ((uint8_t *) &packetBuffer, length);
        Serial.println((char *)packetBuffer);

        String msg = String(packetBuffer);

        digitalWrite(TEST_PIN, 1);
        delay(10);
        digitalWrite(TEST_PIN, 0);

        ParseReply (msg);

        haveReply = true;

        digitalWrite(TEST_PIN, 1);
        delay(20);
        digitalWrite(TEST_PIN, 0);

      }
    }
  }

  if (Serial.available() > 0)
  {
    // read the incoming byte:
    uint8_t incomingByte = Serial.read();

    switch (incomingByte)
    {
      case 'n':
         SendCommand (onCommand);
         break;

      case 'f':
        SendCommand (offCommand);
        break;

      case 'd':
        SendCommand (stateCommand);
        break;

        default:
        break;
    }
  }

  if (digitalRead (SW_INPUT) == 1)
  {
    Serial.println ("Switch active");
    if (awaitingReply == false)
    {
      haveReply = false;
      digitalWrite(TEST_PIN, 1);
      delay(50);
      digitalWrite(TEST_PIN, 0);
      SendCommand (stateCommand);
      awaitingReply = true;
    }
    else
    {
      digitalWrite(TEST_PIN, 1);
      delay(1);
      digitalWrite(TEST_PIN, 0);

      if (haveReply == true)
      {
        // We have the reply, it's already been parsed

        // Toggle the state

        digitalWrite(TEST_PIN, 1);
        delay(30);
        digitalWrite(TEST_PIN, 0);

        ToggleLamp ();

        // Now wait until the user lets go, to prevent constant toggling

        delay (100);

        while (digitalRead (SW_INPUT) == 1)
        {
          delay (10);
        }

        Serial.println ("Let go of switch");

        delay (100);

        haveReply = false;
        awaitingReply = false;
      }
    }
  }

  counter++;

}

void tick()
{
  timer++;
}


void SendCommand (const char * command [])
{
  //Serial.println("Encrypting...");
  // First copy the command locally

  char cmdMessage [250];

  //Serial.println("Reading");

  strcpy((char *)&cmdMessage, (const char *) * command);
  uint8_t messageLength = strlen(cmdMessage);

  //Serial.println(cmdMessage);
  //Serial.println("Length" + messageLength);

  //Serial.println("Ready to encrypt");
  EncryptMessage ((uint8_t *) &cmdMessage);

  // Now send it over UDP

  //udp.begin ();
  udp.beginPacket("192.168.1.45", 9999);
  udp.write(cmdMessage);
  udp.endPacket();

  // The light bulb should reply now
  // This is handled in the main loop

}


/**
Read the state of the lamp, and togggle it
*/
void ToggleLamp (void)
{
  lampState = !lampState;

  digitalWrite(TEST_PIN, 1);
  delay(60);
  digitalWrite(TEST_PIN, 0);

  Serial.println ("Set lamp to " + String (lampState));

  if (lampState == true)
  {
    SendCommand (onCommand);
  }
  else
  {
    SendCommand (offCommand);
  }
}

/**
 * Encrypt the message using the TPLink protocol. Encryption is performed in place.
 */
void EncryptMessage (uint8_t * message)
{
  uint8_t qkey = 0xAB;

  while (*message)
  {
    uint8_t a = *message ^ qkey;
    qkey = a;

    *message = a;      // Stick it back in the array
    message++;
  }
}

void DecryptMessage (uint8_t * message, uint16_t numBytes)
{
  uint8_t qkey = 0xAB;

  for (uint16_t index = 0; index < numBytes; index++)
  {
    uint8_t a = *message ^ qkey;
    qkey = *message;

    *message = a;      // Stick it back in the array
    message++;
  }

  *message = '\0';

}

void ParseReply (String message)
{
  // Have the decruypted message, parse it to get the state
  // Don't care about anything else for now

  //Serial.print ("Parsing: ");
  //Serial.println (message);

  int16_t findIndex = FindText ("on_off", message);

  if (findIndex > 0)
  {
    //Serial.print ("Find index ");
    //Serial.print (findIndex);  // message [index + 3])
    //Serial.print (" character ");
    //Serial.println (message [findIndex + 8]);

    if (message [findIndex + 8] == '0')
    {
      lampState = false;
    }
    else
    {
      lampState = true;
    }

    Serial.println ("Read lamp state " + String(lampState));
  }
}

void PrintWifiStatus()
{
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.print("MAC Address ");
  Serial.println(WiFi.macAddress());
  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


int16_t FindText(String substr, String mainStr)
{
  //return 0;

  //Serial.print ("Searching " + mainStr);
  //Serial.println (" for " + substr);

  //int len = strlen (substr.c_str());

  //Serial.println ();

  //Serial.println ("Main str length " + mainStr.length());

  int16_t foundpos = -1;

  for (int i = 0; i <= mainStr.length () - substr.length (); i++)
  {
    if (mainStr.substring (i, substr.length() + i) == substr)
    {
      foundpos = i;
    }
  }

  //Serial.println (foundpos);

  return foundpos;
}