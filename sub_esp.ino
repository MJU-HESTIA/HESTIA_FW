#include <DHT.h>
#include <DHT_U.h>

#include "BluetoothSerial.h"
#include <Arduino.h>
#include "PinDefinitionsAndMore.h"
#include <IRremote.hpp>
#include <DHT.h>

#define DECODE_NEC  
#define DHTPIN 19
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);



bool airConditionerWarningSent = false;
uint8_t saddress = 0x4;
uint8_t sCommand = 0x8;
uint8_t sRepeats = 0;

unsigned long previousMillisTemperature = 0;
unsigned long previousMillisGas = 0;
unsigned long previousMillisLight = 0;
unsigned long t = 0;

const long intervalTemperature = 12000;  // 12초 간격
const long intervalGas = 13000;  // 13초 간격
const long intervalLight = 11000;  // 11초 간격

BluetoothSerial SerialBT;

void setup() {
  Serial.begin(115200);  // Serial Monitor와 통신
  Serial2.begin(9600, SERIAL_8N1, 16, 17);  // Serial2: 라즈베리파이와 통신 (TX: GPIO 17, RX: GPIO 16)
  dht.begin();  // DHT 센서 초기화
  IrSender.begin();  // IR 송신기 초기화
  pinMode(34, INPUT);  // 가스 센서 핀 설정
  pinMode(27, INPUT);  // 조도 센서 핀 설정
  pinMode(LED_BUILTIN, OUTPUT);
  IrSender.begin(); 
  SerialBT.begin("esp32");
  
}

void loop() {
  unsigned long currentMillis = millis();

  // PC에서 ESP32쪽으로 데이터 전송 시
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }

  // 블루투스 통신으로 ESP32로 데이터 입력 시
  if (SerialBT.available()) {
    String cmd = SerialBT.readStringUntil('\n');
    processBluetoothCommand(cmd);
  }

  // 온습도 데이터를 2초마다 읽고 라즈베리파이로 전송
  if (currentMillis - previousMillisTemperature >= intervalTemperature) {
    previousMillisTemperature = currentMillis;
    sendTemperatureHumidityData();
  }

  // 가스 센서 데이터를 3초마다 읽고 라즈베리파이로 전송
  if (currentMillis - previousMillisGas >= intervalGas) {
    previousMillisGas = currentMillis;
    sendGasData();
  }

  // 조도 센서 데이터를 2.5초마다 읽고 라즈베리파이로 전송
  if (currentMillis - previousMillisLight >= intervalLight) {
    previousMillisLight = currentMillis;
    sendLightData();
  }


  if (Serial2.available()) {
    String dataFromPi = Serial2.readStringUntil('\n');
    Serial.println("Received from Pi: " + dataFromPi);  // Arduino Serial Monitor에 출력
    if (dataFromPi == "tv_on"){
      IrSender.sendNEC(saddress, sCommand, sRepeats);
    }
    else if (dataFromPi == "airconditioner_on"){
      IrSender.sendNEC(saddress, sCommand, sRepeats);
    }
    else if (dataFromPi == "airpurifier_on"){
      IrSender.sendNEC(saddress, sCommand, sRepeats);
    }
    else if (dataFromPi == "tv_off"){
      IrSender.sendNEC(saddress, sCommand, sRepeats);
    }
    else if (dataFromPi == "airconditioner_off"){
      IrSender.sendNEC(saddress, sCommand, sRepeats);
    }
    else if (dataFromPi == "airpurifier_off"){
      IrSender.sendNEC(saddress, sCommand, sRepeats);
    }
    else if (dataFromPi == "led_on"){
      IrSender.sendNEC(saddress, sCommand, sRepeats);
    }
    else if (dataFromPi == "led_off"){
      IrSender.sendNEC(saddress, sCommand, sRepeats);
    }
        
  }

  // 시리얼 모니터에서 데이터 수신
  if (Serial.available()) {
    String dataFromSerial = Serial.readStringUntil('\n');
    Serial2.println(dataFromSerial);  // 라즈베리파이로 전송
  }
  
}

// 블루투스 명령 처리
void processBluetoothCommand(String cmd) {
  if (cmd == "tvon" || cmd == "tvoff") {
    IrSender.sendNEC(saddress, sCommand, sRepeats);
  } else if (cmd == "ledon") {
    digitalWrite(32, HIGH);
  } else if (cmd == "ledoff") {
    digitalWrite(32, LOW);
  } else if (cmd == "airconditioneron") {
    IrSender.sendLG(0x88, 0x40B0, sRepeats);
    SerialBT.print("stop");
  } else if (cmd == "airconditioneroff") {
    IrSender.sendLG(0x88, 0xC005, sRepeats);
  } else if (cmd == "airpurifieron" || cmd == "airpurifieroff") {
    IrSender.sendNEC(saddress, sCommand, sRepeats);
  }
}

// 온도와 습도 데이터를 전송
void sendTemperatureHumidityData() {
  int t = dht.readTemperature();

  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  String packet = "<T^" + String(t) + ">";
  Serial2.println(packet);
  Serial.println("Sent to Pi: " + packet);
}

// 가스 센서 데이터를 전송
void sendGasData() {
  String packet = "<G^" + String(!digitalRead(34)) + ">";
  Serial2.println(packet);
  Serial.println("Sent to Pi: " + packet);
}

// 조도 센서 데이터를 전송
void sendLightData() {

  String packet = "<L^" + String(!digitalRead(27)) + ">";
  Serial2.println(packet);
  Serial.println("Sent to Pi: " + packet);
}