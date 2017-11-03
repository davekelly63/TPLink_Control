

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// WEB Information
const char* MY_SSID = "TALKTALK531F4C"; 
const char* MY_PWD =  "parsimonious";

const char * onCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\": {\"transition_light_state\": {\"ignore_default\": 1, \"on_off\": 1, \"transition_period\": 0, \"brightness\": 25}}}"};
const char * offCommand [] = {"{\"smartlife.iot.smartbulb.lightingservice\": {\"transition_light_state\": {\"ignore_default\": 1, \"on_off\": 0, \"transition_period\": 0}}}"};

WiFiUDP udp;

void setup() {

  // put your setup code here, to run once:
  Serial.begin (115200);
  ConnectWifi();
}

void loop() {
  // put your main code here, to run repeatedly:

  // if there's data available, read a packet
  int packetSize = udp.parsePacket();

  if (packetSize > 0)
  {
    if (udp.remotePort() == 9999)
    {
      // it is from the bulb

      char packetBuffer [250];
      int length = udp.read(packetBuffer, 250);
      if (length > 0)
      {
        Serial.println ("Data received from bulb");
        // Now decrypt it
      }
    }
  }
}


void ConnectWifi()
{
  Serial.println();
  Serial.println("Set to STA mode");
  WiFi.mode(WIFI_STA);
  Serial.print("Connecting to "+*MY_SSID);
  
  WiFi.begin(MY_SSID, MY_PWD);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected ");
  Serial.println("");  

  PrintWifiStatus ();

  SendCommand (onCommand);
  
}//end connect


void SendCommand (const char * command [])
{
  Serial.println("Encrypting...");
  // First copy the command locally

  char cmdMessage [250];
  
  Serial.println("Reading");

  strcpy((char *)&cmdMessage, (const char *) * command);
  unsigned int messageLength = strlen(cmdMessage);

  Serial.println(cmdMessage);
  Serial.println("Length" + messageLength);
      
  Serial.println("Ready to encrypt");
  EncryptMessage ((char *) &cmdMessage);

  // Test dump

  /*for (unsigned int index = 0; index < messageLength; index++)
  {
    Serial.print(cmdMessage[index], HEX);
    Serial.print (" ");
  }

  Serial.println();
  Serial.println("Finished");*/

  // Now send it over UDP

  //udp.begin ();
  udp.beginPacket("192.168.1.45", 9999);
  udp.write(cmdMessage);
  udp.endPacket();

  // The light bulb should reply now
  // This is handled in the main loop
  
}

void EncryptMessage (char * message)
{
  unsigned int qkey = 0xAB;
    
  while (*message)
  {
    unsigned char a = *message ^ qkey;
    qkey = a;

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

void SendTPLinkCommand (unsigned char *message)
{
  WiFiClient http;
  if (http.connect("192.168.1.45", 9999))     // Static IP
  {
    Serial.println("WiFi Client connected ");
    
  }
}
