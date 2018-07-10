#include <SPI.h>
#include <SD.h>
#include <FlowMeter.h>
#include <Ultrasonic.h>
#include "index.h" //Our HTML webpage contents with javascripts

// Websocket for Node-RED
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

//------------------------------- Define Websocket connection -----------------//
WebSocketsClient webSocket;
bool connected = false;

// ------------------------------ Call library ------------------------------- //
FlowMeter Meter = FlowMeter(4);
Ultrasonic ultrasonic(0, 2);
// -------------------------------------------------------------------------- //

#define control_pump D0        //Control Pump 
int mode_select = 0;
int pump_statefromweb ;

File myFile;

const unsigned long period = 1000; //Flowmeter Interrupt
unsigned long sd_millis; //Count time to save value to sdcard
unsigned long control_millis;

const int buttonPin = 10;     // pin push button
const int buttonSW = 9;

int currentButtonState = LOW; // state button now
int previousButtonState = LOW;        // state Button previous
bool isLedOn = false;                 // state led
int currentButtonM = LOW; // ค่าสถานะปัจจุบนของปุ่ม
int previousButtonM = LOW;        // ค่าสถานะของปุ่มกดครั้งที่แล้ว

//----------------------------- WebSocket Event ---------------------//

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected! \n");
      connected = false;
      break;
    case WStype_CONNECTED:
      {
        Serial.printf("[WSc] Connected %s\n", payload);
        webSocket.sendTXT("Connected " + String(ESP.getChipId()));
        connected = true;
      }
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text %s\n", payload );

      if (true) {
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& root = jsonBuffer.parseObject((char *)payload); // ตีความ Srting ของ Json ที่ส่งเข้า

        if (!root.success()) {
          Serial.println("parseObjse(0 failed");
        } else {
          String modesss = root["mode"];
          String pump_state = root["pump_state"];
          String nodeid = root["nodeid"];

          String myNodeId = String(ESP.getChipId());  //ส่งnodeid ไปด้วยเพื่อให้รู้ว่ามาจากตัวไหน
          if (nodeid != NULL && nodeid == myNodeId) { // ตรวจว่า nodeid ว่าตรงกันไหม
            Serial.println(modesss);
            if (modesss == "manual") {
              mode_select = 0;
            }
            if (pump_state == "on") {
              pump_statefromweb = 1;
            }
            if (pump_state == "off") {
              pump_statefromweb = 0;
            }
            if (modesss == "auto") {
              mode_select = 1;
            }
          }
          String str = "node: " + myNodeId + "-> Pump" + " " + pump_state;
          webSocket.sendTXT(str);

        }
      }

      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);
      break;

  }
}



//==============================================================
//                  SETUP
//==============================================================

void setup() {
  Serial.begin(115200);

  WiFi.begin("Maka", "");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.print("WiFi Connected IP Address ");
  Serial.println(WiFi.localIP());

  webSocket.begin("172.19.201.179", 1880);
  //webSocket.begin("http://node-red-server-farmcontrol.c9users.io/#flow/171f27e8.416c68", 8080);
  webSocket.onEvent(webSocketEvent);
  Serial.println(ESP.getChipId());


  pinMode(buttonPin, INPUT);
  pinMode(buttonSW, INPUT);
  pinMode(control_pump, OUTPUT);
  sd_millis = millis();
  control_millis = millis();

  // enable a call to the 'interrupt service handler' (ISR) on every rising edge at the interrupt pin
  // do this setup step for every ISR you have defined, depending on how many interrupts you use
  attachInterrupt(4, MeterISR, RISING);

  attachInterrupt(9, control_manuel, CHANGE   ); // pump
  attachInterrupt(10, control_manuel_pump, CHANGE   ); // in to manuel

  // sometimes initializing the gear generates some pulses that we should ignore
  Meter.reset();
  //checkSD();

}



//==============================================================
//                     LOOP
//==============================================================

unsigned long next = millis();

void loop() {
  webSocket.loop();

  if (millis() - next > period) {
    next = millis();

    //delay(period);    // wait between output updates
    Meter.tick(period);   // process the (possibly) counted ticks
    //Serial.println("Currently Flow: " + String(Meter.getCurrentFlowrate()) + " l/min, " + "Total Flow Volume: " + String(Meter.getTotalVolume()));
    Serial.println("Currently Ultrasonic: " + String(ultrasonic.calUltra()));

    if (connected == false) {
      Serial.println("Connection Lost! go to offine mode");
      if (currentButtonM == LOW ) {
        Serial.println("in to control_Auto_pump");

        if (ultrasonic.calUltra() > 35 or ultrasonic.calUltra() < 20) { //Auto
          digitalWrite(control_pump, LOW); //Auto
          Serial.println("Status Pump :LOW");
        } else {
          digitalWrite(control_pump, HIGH); //Auto
          Serial.println("Status Pump :HIGH");
        }
      }

      if (currentButtonM == HIGH ) {

        Serial.println("in to control_manuel_pump");

        if (currentButtonState == HIGH) {
          digitalWrite(control_pump, HIGH);
          Serial.println("control pump ON");

        } else if (currentButtonState == LOW) {
          digitalWrite(control_pump, LOW);
          Serial.println("control pump OFF");
        }
      }
    }

    if (connected == true) {
      if (mode_select == 1) {

        Serial.println("in to control_Auto_pump from web");

        if (ultrasonic.calUltra() > 35 or ultrasonic.calUltra() < 20) { //Auto
          digitalWrite(control_pump, LOW); //Auto
          Serial.println("Status Pump :LOW");
        } else {
          digitalWrite(control_pump, HIGH); //Auto
          Serial.println("Status Pump :HIGH");
        }
      }
     else if (mode_select == 0) {

      Serial.println("in to control_manuel_pump from web ");
      if (pump_statefromweb == 1) {
        digitalWrite(control_pump, HIGH);
        Serial.println("control pump ON");
      } else if (pump_statefromweb == 0) {
        digitalWrite(control_pump, LOW);
        Serial.println("control pump OFF");
      }
    }
  }
 }

  String nodeId = String(ESP.getChipId());

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();

  root["nodeid"] = nodeId;
  root["flow"] = String(Meter.getCurrentFlowrate());
  root["Ultrasonic"] = String(ultrasonic.calUltra());

  String str;
  root.printTo(str);

  webSocket.sendTXT(str);
  
}

// unsigned long now_millis = millis();
// if (now_millis - sd_millis > 60 * 15 * 1000) {
//   writeData();
//   sd_millis = now_millis;
// }



//==============================================================
//                     OTHET FUNCTION
//==============================================================

// --------------------------------------------------------------
void control_manuel() {
  currentButtonM = digitalRead(buttonSW);
  Serial.print("in to control manule: " );
  Serial.println(currentButtonM);

}

void control_manuel_pump() {
  currentButtonState = digitalRead(buttonPin);
  Serial.print("in to control manule pump: " );
  Serial.println(currentButtonState);
}


// -------------------------------- Function Flow meter --------------------------------- //
// define an 'interrupt service handler' (ISR) for every interrupt pin you use //
void MeterISR() {
  // let our flow meter count the pulses
  Meter.count();
}

// ----------------------------------- SD Card ------------------------------------ //

// void checkSD() {
//   Serial.print("Initializing SD card...");

//   if (!SD.begin(4)) {
//     Serial.println("initialization failed!");
//     return;
//   }
//   Serial.println("initialization done.");
// }

// void writeData() {
//   // open the file. note that only one file can be open at a time,
//   // so you have to close this one before opening another.
//   myFile = SD.open("test.txt", FILE_WRITE);

//   // if the file opened okay, write to it:
//   if (myFile) {
//     Serial.println("Writing to test.txt...");
//     myFile.println("Currently Flow: " + String(Meter.getCurrentFlowrate()) + " l/min, " + "Total Flow Volume: " + String(Meter.getTotalVolume()));
//     myFile.println("Currently Ultrasonic: " + String(ultrasonic.calUltra()));
//     // close the file:
//     myFile.close();
//     Serial.println("Writing to test.txt done.");
//     readData();
//   } else {
//     // if the file didn't open, print an error:
//     Serial.println("error opening test.txt");
//   }
// }

// void readData() {

//   // re-open the file for reading:
//   myFile = SD.open("test.txt");
//   if (myFile) {
//     Serial.println("test.txt:");

//     // read from the file until there's nothing else in it:
//     while (myFile.available()) {
//       Serial.write(myFile.read());
//     }
//     // close the file:
//     myFile.close();
//   } else {
//     // if the file didn't open, print an error:
//     Serial.println("error opening test.txt");
//   }
// }


