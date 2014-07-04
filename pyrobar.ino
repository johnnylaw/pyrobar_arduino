#include <SPI.h>
#include <WiFi.h>

#define ZONE_COUNT 9
#define RED 0
#define GREEN 0
#define BLUE 0

#define BUFFER_SIZE_FREQUENCY 256
#define BUFFER_SIZE_SOUND 16

#define SSID "EmbassyNorth"
#define WIFI_PASS "netpositive"

int frequencyBuffers[ZONE_COUNT][3][BUFFER_SIZE_FREQUENCY];
int soundBuffers[ZONE_COUNT][3][BUFFER_SIZE_SOUND];

// Listen on default port 5555, the webserver on the Yun
// will forward there all the HTTP requests for us.

float soundSensitivity;
float frequency;

float lastMillis;
float currentFreqBufferIndex;

int wifiStatus = WL_IDLE_STATUS;

void setup() {  
  setUpWifi();
  Serial.begin(9600);
  while(!Serial);

  printLocalIP();
//
//  pinMode(13, OUTPUT);
//  digitalWrite(13, LOW);
//  digitalWrite(13, HIGH);
  
  soundSensitivity = 0.0;
  frequency = 0.00025;
  lastMillis = millis();
  currentFreqBufferIndex = 0.0;
}

void loop() {
  setPins();
  delay(30); // Loop takes about 20ms => 50ms total => 20 Hz
}

void setUpWifi() {
  wifiStatus = WiFi.begin(SSID, WIFI_PASS);
}

int advanceFrequencyIndex() {
  float currentMillis = millis();
  float timeElapsed = currentMillis - lastMillis;
  Serial.print("\nTime elapsed: ");
  Serial.println(timeElapsed);

  lastMillis = currentMillis;
  currentFreqBufferIndex += (timeElapsed * frequency);
  return (int)fmod(currentFreqBufferIndex, (float)BUFFER_SIZE_FREQUENCY);
}

void setPins() {  
  int frequencyIndex = advanceFrequencyIndex();  
  int soundIndex = min(soundSensitivity * BUFFER_SIZE_SOUND, BUFFER_SIZE_SOUND);

  for(int zone = 0; zone < ZONE_COUNT; zone++) {
    int redPin = 16 * max(frequencyBuffers[zone][RED][frequencyIndex], soundBuffers[zone][RED][soundIndex]);
    int greenPin = 16 * max(frequencyBuffers[zone][GREEN][frequencyIndex], soundBuffers[zone][GREEN][soundIndex]);
    int bluePin = 16 * max(frequencyBuffers[zone][BLUE][frequencyIndex], soundBuffers[zone][BLUE][soundIndex]);
  }
}

void printLocalIP() {
  if ( wifiStatus != WL_CONNECTED) { 
    Serial.println("Couldn't get a wifi connection");
    while(true);
  } else {
      Serial.print("\nConnected to the network, IP: ");
      Serial.println(WiFi.localIP());
  }
}

void printMacAddress() {
  // the MAC address of your Wifi shield
  byte mac[6];                     

  // print your MAC address:
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
}

//  Serial.print("\n\nLIGHT AT CURRENT SOUND INDEX (");
//  Serial.print(soundIndex);
//  Serial.print("): ");
//  Serial.print(", ");
//  Serial.print(", ");
//
//  Serial.print("LIGHT AT CURRENT FREQ INDEX (");
//  Serial.print((int)currentFreqBufferIndex);
//  Serial.print("): ");
//  Serial.print(", ");
//  Serial.print(", ");
//  
//  Serial.print("Time elapsed: ");
//  Serial.println(currentFreqBufferIndex);
//  Serial.print(", ");
//  Serial.print(", ");

