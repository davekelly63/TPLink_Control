
#include <stdbool.h>
#include <stdint.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// WEB Information
const char* MY_SSID = "TALKTALK531F4C";
const char* MY_PWD =  "parsimonious";

//const char * onCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\": {\"transition_light_state\": {\"ignore_default\": 1, \"on_off\": 1, \"transition_period\": 0, \"brightness\": 25}}}"};
const char * onCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\": {\"transition_light_state\": {\"ignore_default\": 1, \"on_off\": 1, \"transition_period\": 0}}}"};
const char * offCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\": {\"transition_light_state\": {\"ignore_default\": 1, \"on_off\": 0, \"transition_period\": 0}}}"};
const char * detailsCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\":{\"get_light_details\":{}}}"};
const char * stateCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\":{\"get_light_state\":{}}}"};

WiFiUDP udp;

IPAddress ip(192, 168, 1, 46); // where xx is the desired IP Address
IPAddress gateway(192, 168, 1, 1); // set gateway to match your network

IPAddress subnet(255, 255, 255, 0); // set subnet mask to match your network

bool lampState = false;

#define SW_INPUT    5
#define LED         13          // Connected to D7

void setup() 
{

  // put your setup code here, to run once:
  pinMode (SW_INPUT, INPUT);                   // Switch input
  
  Serial.begin (115200);
  ConnectWifi();

  delay(100);

  Serial.println("Started");
  
  // Always turn off on startup, so we know the state variable is correct
  //SendCommand (onCommand);
}

void(* resetFunc) (void) = 0;//declare reset function at address 0

void loop() 
{
  // put your main code here, to run repeatedly:

  // if there's data available, read a packet
  uint16_t packetSize = udp.parsePacket();

  packetSize = udp.available ();
  
  if (packetSize > 0)
  {
    Serial.println ("Data available from " + udp.remotePort());
    
    if (udp.remotePort() == 9999)
    {
      Serial.println ("Data available. Bang");
      // it is from the bulb

      uint8_t packetBuffer [250];
      uint16_t length = udp.read (packetBuffer, sizeof(packetBuffer));
      if (length > 0)
      {
        Serial.println ("Data received from bulb "); // + length);
        // Now decrypt it

        DecryptMessage ((uint8_t *) &packetBuffer, length);
        Serial.println((char *)packetBuffer);
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

      case 'r':
        resetFunc ();
        break;
        
      default:
        break;
    }
  }

  if (digitalRead (SW_INPUT) == 1)
  {
    // Toggle the state

    lampState = !lampState;

    if (lampState == true)
    {
      SendCommand (onCommand);
    }
    else
    {
      SendCommand (offCommand);
    }

    // Now wait until the user lets go, to prevent constant toggling

    delay (50);
    
    while (digitalRead (SW_INPUT) == 1)
    {
      delay (10);  
    }

    delay (100);
  }
}


void ConnectWifi()
{
  uint32_t startTime = millis ();
  
  Serial.println();
  Serial.println("Set to STA mode");
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to " + *MY_SSID);

  WiFi.config(ip, gateway, subnet);
  
  WiFi.begin(MY_SSID, MY_PWD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(100);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected ");
  Serial.println("");

  Serial.print ("Time to connect to network ");
  Serial.print ((millis() - startTime), DEC);
  Serial.println (" ms");
  
  PrintWifiStatus ();

  Serial.println ("udp Listen " + udp.begin (9999));

  SendCommand (onCommand);
  SendCommand (onCommand);
    
}//end connect


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