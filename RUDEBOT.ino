// CAVEATS
// 1) If you don't send data to the client during periods of inactivity, the wifi
//    library will disconnect. Hence the keepalive code.
// 2) If you don't send the client actual responses to data that is sent after a very
//    very short period of time, the wifi library will disconnect.
// 3) In both cases, if these conditions are met, the wifi library will not allow new
//    connections to come in. I believe this is a bug!

#include <SPI.h>
#include <WiFi.h>
#include "DualMC33926MotorShield.h"
#include "keys.h"

// Every 60 seconds shoot across a keepalive due to CAVEAT (2)
#define KEEPALIVE 60000

DualMC33926MotorShield md;

void stopIfFault()
{
  if (md.getFault())
  {
    Serial.println("fault");
    while(1);
  }
}

const char forwardk = 'k';
const char reversek = 'j';
const char leftk = 'h';
const char rightk = 'l';

const char forwardk2 = 'w';
const char reversek2 = 's';
const char leftk2 = 'a';
const char rightk2 = 'd';

boolean alreadyConnected = false;
byte mac[6];
WiFiClient organic = NULL;
unsigned long keepalive = millis();
unsigned long lastc = millis();
int speed = 100;
int status = WL_IDLE_STATUS;
// How many milliseconds do we drive for in between commands?
int thrust_drivetime = 666;
int yaw_drivetime = 666;
int drivetime = 666;

WiFiServer server(8888);

void setup() {
  md.init();

  //Initialize serial and wait for port to open:
  Serial.begin(9600); 
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present"); 
    // don't continue:
    while(true);
  } 

  // Spit out MAC 
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5],HEX);
  Serial.print(":");
  Serial.print(mac[4],HEX);
  Serial.print(":");
  Serial.print(mac[3],HEX);
  Serial.print(":");
  Serial.print(mac[2],HEX);
  Serial.print(":");
  Serial.print(mac[1],HEX);
  Serial.print(":");
  Serial.println(mac[0],HEX);
  
  // Connect
  connectWifi();
}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();

  if (client) {
    if (organic && (organic != client)) {
      Serial.println("Sorry, only one organic lifeform at a time");
      server.write("Sorry, only one organic lifeform at a time\n");
      alreadyConnected = false;
      client.flush();
      client.stop();
    }    
    
    // New clients
    if (!alreadyConnected) {
      Serial.println("New lifeform detected");
      server.write("New lifeform detected\n");
      alreadyConnected = true;
      client.flush();
      organic = client;
    }

    // Kill old clients 
    if (organic && !organic.connected()) {
      Serial.println("Severing mind link");
      organic.flush();
      organic.stop();
      organic = NULL;
      alreadyConnected = false;
    }

    if (client.available()) {
      char c = client.read();
      
      switch (c) {
        case forwardk:
        case forwardk2:
          drivetime = thrust_drivetime;
          md.setSpeeds(speed,speed);          
          break;
        case reversek:
        case reversek2:
          drivetime = thrust_drivetime;
          md.setSpeeds(-speed,-speed);
          break;
        case leftk:
        case leftk2:
          drivetime = yaw_drivetime;
          md.setSpeeds(-speed,speed);
          break;
        case rightk:
        case rightk2:
          drivetime = yaw_drivetime;
          md.setSpeeds(speed,-speed);
          break;
        case '1':
          speed = 100;
          thrust_drivetime = 600;
          yaw_drivetime = 300;
          break;
        case '2':
          speed = 133;
          thrust_drivetime = 533;
          yaw_drivetime = 266;
          break;
        case '3':
          speed = 166;
          thrust_drivetime = 466;
          yaw_drivetime = 233;
          break;
        case '4':
          speed = 200;
          thrust_drivetime = 400;
          yaw_drivetime = 200;
          break;
        
      } 
      // Time of last command
      lastc = millis();

      // If the server doesn't send feedback, the wifi library disconnects. WTF!?
      // I'd like to remove this, but it could be useful for the client to know when
      // an action has executed.
      server.write(c);
    } else { 
      if (millis() - lastc > drivetime) {
        md.setSpeeds(0,0);
      }
      // keepalive (annoying!!)
      if ( (millis() - keepalive) > KEEPALIVE) {
        if (organic)
          organic.flush();
        server.write(".");
        Serial.println("Ping!");
        keepalive = millis();
      }
    }
  } else {
    //Serial.println("dead client: " + String(client));
    //Serial.println("server status: " + String(server.status()));
  }
}

void connectWifi() {
  while (status != WL_CONNECTED) { 
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);
    if (status != WL_CONNECTED) { 
      Serial.println("Couldn't get a wifi connection, retrying...");
    } else {
      server.begin();
      printWifiStatus();
    }
  } 
}

void printWifiStatus() {
  // Firmware version?
  Serial.println("Firmware Version: " + String(WiFi.firmwareVersion()));
  
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

