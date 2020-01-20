#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// #define DEBUG

#define LED_OUT    LED_BUILTIN
#define DTATA1     5    // columns data pins
#define LATCH      12   // columns,rows latch pins
#define CLOCK1     4    // columns clock pins
#define DATA2      14   // rows data pin 
#define CLOCK2     13   // rows clock pin

//Creating UDP Listener Object. 
WiFiUDP UDPServer;
unsigned int UDPPort = 6868;
unsigned int clientPort = 6969;
const char* ssid = "xxxxxx"; //your wifi ssid
const char* password = "yyyyyy";  //your wifi password 
const char* ssid_ap = "SmartLedMatrix";
const char* password_ap = "esp8266Matrix";

unsigned int d;    // Data to be sent to the shift reg.
int dir =0;        // Direction of walking 1.
char buf[12];      // General purpose buffer.

unsigned long previousMillis = 0;
const unsigned long interval = 1;

const unsigned long MAX_INTERVAL = 1050;
const unsigned long DELTA_INTERVAL = 200;
unsigned long previousMillis2 = 0;
unsigned long interval2 = MAX_INTERVAL;

int ledState = LOW;

const int ROW_CNT = 24;
const int PACKAGE_SIZE = ROW_CNT;
byte packetBuffer[ROW_CNT];
byte colsVal[ROW_CNT];
int8_t shiftSpeed = 0;

#define ROW_CNT 24
#define COL_CNT 8

void writeCell(int cols, int rows){
  digitalWrite(LATCH, LOW);
  shiftOut(DTATA1, CLOCK1, MSBFIRST, (0x00ff0000 & rows)>>16);
  shiftOut(DTATA1, CLOCK1, MSBFIRST, (0x0000ff00 & rows)>>8);
  shiftOut(DTATA1, CLOCK1, MSBFIRST,  0x000000ff & rows);
  shiftOut(DATA2,  CLOCK2, MSBFIRST, cols);
  digitalWrite(LATCH, HIGH);
}

void drawLed(){
  int rows = 0x00000001;
  for(int i=0; i< PACKAGE_SIZE; i++){
      writeCell(~colsVal[i],rows);
      rows <<= 1;
      delayMicroseconds(150);
  }
}

void processPostData(){
  int cb = UDPServer.parsePacket();
  if (cb) {
    UDPServer.read(packetBuffer, PACKAGE_SIZE);
    // UDPServer.flush();
    // IPAddress addr = UDPServer.remoteIP();
    // UDPServer.beginPacket(addr, clientPort);
    // UDPServer.write("ok");
    // UDPServer.endPacket();

#ifdef DEBUG
    Serial.println("receive: ");
    for (int y = 0; y < ROW_CNT; y++){
      Serial.print(packetBuffer[y]);
      Serial.print("\n");
    }
    Serial.print("from: ");
    Serial.println(addr);
#endif
    if(  packetBuffer[0]!='s' 
      && packetBuffer[1]!='t' 
      && packetBuffer[2]!='a' 
      && packetBuffer[3]!='t'
      && packetBuffer[4]!='e'){
        for(int i=0; i< PACKAGE_SIZE; i++){
          colsVal[i] = packetBuffer[i];
          // Serial.println(packetBuffer[i]);
        }
        shiftSpeed = 0;
      }else{
        shiftSpeed = packetBuffer[5];
        interval2 = MAX_INTERVAL - abs(shiftSpeed)*DELTA_INTERVAL;
        // Serial.println(shiftSpeed);
      }
  }
}

void setup()
{
  pinMode(DTATA1,OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(CLOCK1,OUTPUT);
  pinMode(DATA2, OUTPUT);
  pinMode(CLOCK2,OUTPUT);
  d=1; 
  Serial.begin(9600);

  pinMode(LED_OUT, OUTPUT);
  digitalWrite(LED_OUT, LOW);

#if 0
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
#else
  WiFi.softAP(ssid_ap, password_ap);
  Serial.println("");

  Serial.println();
  Serial.print("Server IP address: ");
  Serial.println(WiFi.softAPIP());
  Serial.print("Server MAC address: ");
  Serial.println(WiFi.softAPmacAddress());
#endif

  UDPServer.begin(UDPPort); 

  digitalWrite(LED_OUT, HIGH);

}

void test(){
    writeCell(0xF9, d);

    if (!dir) d<<=1; else d>>=1; // Shift

    if (d&0x800000) dir=1;           // Set direction.
    if (d&0x000001) dir=0;
}

void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    // if (ledState == LOW)
    //   ledState = HIGH;  // Note that this switches the LED *off*
    // else
    //   ledState = LOW;   // Note that this switches the LED *on*
    // digitalWrite(LED_OUT, ledState);
    processPostData();
  }

  if(currentMillis - previousMillis2 > interval2){
    previousMillis2 = currentMillis; 
    if( shiftSpeed < 0 ){
      byte tmp = colsVal[0];
      for(int i=0; i < PACKAGE_SIZE-1; i++){
        colsVal[i] = colsVal[i+1];
      }
      colsVal[PACKAGE_SIZE-1] = tmp;
    }else if (shiftSpeed > 0)
    {
      byte tmp = colsVal[PACKAGE_SIZE-1];
      for(int i=PACKAGE_SIZE-1; i>0 ; i--){
        colsVal[i] = colsVal[i-1];
      }
      colsVal[0] = tmp;
    }    
    // Serial.println(interval2);
  }

  drawLed();
}
