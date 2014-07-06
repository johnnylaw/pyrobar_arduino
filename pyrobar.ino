#include <SPI.h>
#include <Ethernet.h>
//#include "Adafruit_PWMServoDriver.h"

#define ZONE_COUNT 9
#define RED 0
#define GREEN 0
#define BLUE 0

#define DEFAULT_FREQUENCY 0.00025 // cycles per ms

#define BUFFER_SIZE_FREQUENCY 256
#define BUFFER_SIZE_SOUND 16

#define SOUND "sound"
#define FREQUENCY "frequency"

// Set up frequency-based buffer index
float lastMillis;
float currentFreqBufferIndex = 0.0;

// Set up light buffers
uint8_t frequencyBuffers[ZONE_COUNT][BUFFER_SIZE_FREQUENCY][3];
uint8_t soundBuffers[ZONE_COUNT][BUFFER_SIZE_SOUND][3];
char tempHex[3] = "00";

// Set up defaults
float frequency = DEFAULT_FREQUENCY;
float soundSensitivity = 0.0;
float soundPower = 0.0;

IPAddress ip(10, 1, 10, 101);
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
EthernetServer server(80);

void setup() {  
//  setUpWifi();
  Serial.begin(9600);
  while(!Serial);

  Ethernet.begin(mac, ip);
  Serial.println(Ethernet.localIP());
  
//  wifiServer.begin();
//
//  pinMode(13, OUTPUT);
//  digitalWrite(13, LOW);
//  digitalWrite(13, HIGH);
  
  soundSensitivity = 0.0;
  lastMillis = millis();  
}
/* NOTE for computing buffers on ipad side; use Obj-C of course
arr = (0..50).map{ |x| x / 50.0 }
arr.concat arr[0..-2].reverse
arr.concat (1..155).map{ |x| 0 }
cor = 1.0/256; p arr.map{ |x| ((2 ** (8*x - 8) - cor) * 256).floor.to_s(16).sub(/^(\w)$/, '0\1') }.join
*/
void loop() {
  EthernetClient client = server.available();
  if (client) {
    boolean currentLineIsBlank = true;
    String responseCode = parseRequest(client);
    while (client.connected()) {
      char c = client.read();
      Serial.print(c);
      if (c == '\n' && currentLineIsBlank) {
        client.print("HTTP/1.1 ");
        client.print(responseCode);
        client.println(responseCode == "200" ? " OK" : " BAD REQUEST");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");  // the connection will be closed after completion of the response
        break;
      }
      if (c == '\n') currentLineIsBlank = true;
      else if (c != '\r') currentLineIsBlank = false;
    }
    client.stop();
  }
  soundPower = 0 / 1024.0; // read off of pin input connected to Uno output and divide by whatever (probably 1024)
  setPins();
  if(analogRead(0) > 512) printDiagnostics();
}

int nextFrequencyIndex() {
  float currentMillis = millis();
  float timeElapsed = currentMillis - lastMillis;
//  Serial.print("\nTime elapsed: ");
//  Serial.println(timeElapsed);

  lastMillis = currentMillis;
  currentFreqBufferIndex += (timeElapsed * frequency);
  return (int)fmod(currentFreqBufferIndex, (float)BUFFER_SIZE_FREQUENCY);
}

void setPins() {  
  int frequencyIndex = nextFrequencyIndex();  
  int soundIndex = min(soundSensitivity * soundPower * BUFFER_SIZE_SOUND, BUFFER_SIZE_SOUND);

  for(int zone = 0; zone < ZONE_COUNT; zone++) {
    int redPin = 16 * max(frequencyBuffers[zone][RED][frequencyIndex], soundBuffers[zone][RED][soundIndex]);
    int greenPin = 16 * max(frequencyBuffers[zone][GREEN][frequencyIndex], soundBuffers[zone][GREEN][soundIndex]);
    int bluePin = 16 * max(frequencyBuffers[zone][BLUE][frequencyIndex], soundBuffers[zone][BLUE][soundIndex]);
  }
}

String parseRequest(EthernetClient client) {
  if (client.available()) {
    if(client.readStringUntil(' ') == "GET") {
      if(client.read() == '/') {
        String dataType = client.readStringUntil('/');
        Serial.print("\nData Type: ");
        Serial.println(dataType);
        if(dataType == "zones") return loadBuffer(dataType, client);
        else if(dataType == FREQUENCY) {
          String string = client.readStringUntil(' ');
          frequency = string.toFloat() * 0.001;
          Serial.println(frequency);
          return "200";
        } else if(dataType == SOUND) {
          String string = client.readStringUntil(' ');
          soundSensitivity = string.toFloat();
          Serial.println(soundSensitivity);
          return "200";
        }
      }
    } else {
      return "401";
    }
  }
}

String loadBuffer(String dataType, EthernetClient client) {
  int zone = atoi(client.readStringUntil('/').c_str());
  Serial.print("\nZone: ");
  Serial.print(zone);  
  if(zone >= 0 && zone < ZONE_COUNT) {
    String bufferType = client.readStringUntil('/');
    Serial.print(" ");
    Serial.println(bufferType);
    if(bufferType == "frequency") {
      loadMyBuffer(frequencyBuffers[zone], BUFFER_SIZE_FREQUENCY, client);
      return "200";
    } else if(bufferType == "sound") {
      loadMyBuffer(soundBuffers[zone], BUFFER_SIZE_SOUND, client);
      return "200";
    } else {
      Serial.print("Buffer type must be 'frequency' or 'sound', not ");
      Serial.println(bufferType);
      return "401";
    }
  } else {
    Serial.print("ERROR. NO ZONE ");
    Serial.println(zone);
    return "401";
  }
}

void loadMyBuffer(uint8_t buf[][3], int bufferLength, EthernetClient client) {
  for(int color = 0; color < 3; color++) {
    for(int ptr = 0; ptr < bufferLength; ptr++) {
      tempHex[0] = client.read();
      tempHex[1] = client.read();
      uint8_t value = strtoul(tempHex, NULL, 16); 
      buf[ptr][color] = value;
    }
  }
}

void printDiagnostics() {
  Serial.println("\n\nRed Buffer Values");
  for(int ptr = 0; ptr < 256; ptr++) {
    for(int zone = 0; zone < ZONE_COUNT; zone++) {
//      Serial.print("R: ");
//      Serial.print(frequencyBuffers[zone][ptr][0]);
//      Serial.print(", G: ");
//      Serial.print(frequencyBuffers[zone][ptr][1]);
//      Serial.print(", B: ");
//      Serial.println(frequencyBuffers[zone][ptr][2]);
      Serial.print(frequencyBuffers[zone][ptr][0]);
      Serial.print("\t");
    }
    Serial.println();
  }
  Serial.print("\nSound senstivity: ");
  Serial.print(soundSensitivity);
  Serial.print(", Frequency: ");
  Serial.println(frequency * 1000);
}

//void printMacAddress() {
//  // the MAC address of your Wifi shield
//  byte mac[6];                     
//
//  // print your MAC address:
//  WiFi.macAddress(mac);
//  Serial.print("MAC: ");
//  Serial.print(mac[5],HEX);
//  Serial.print(":");
//  Serial.print(mac[4],HEX);
//  Serial.print(":");
//  Serial.print(mac[3],HEX);
//  Serial.print(":");
//  Serial.print(mac[2],HEX);
//  Serial.print(":");
//  Serial.print(mac[1],HEX);
//  Serial.print(":");
//  Serial.println(mac[0],HEX);
//}

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

