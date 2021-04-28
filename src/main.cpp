#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>
#include "id.h"
#include "sound.h"


#define DAC_PIN 25
#define BUTTON 3
#define LED LED_BUILTIN

bool not_pressed = true;

WiFiUDP udp_client;
WiFiUDP udp_server;

MAX30105 particleSensor;

const byte RATE_SIZE = 10; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;
boolean suspend = true;

TaskHandle_t playerHandle;

void beatPlayer(void *arg) {
    while(1) {
    Serial.println("Start Player");
    for (int i = 0; i < SOUND_SAMPLES; i++){
      dacWrite(DAC_PIN, t[i]); // Square
      delayMicroseconds((60E6)/(SR*beatAvg));
      if(suspend) break;
    }
    dacWrite(DAC_PIN, 0);
    vTaskDelay(((1000*beatAvg)/60.0));
  }
}

void udpSend(String text) {
  udp_client.beginPacket(DEST, PORT);//Inicializa o pacote de transmissao ao IP e PORTA.
  udp_client.println(text);//Adiciona-se o valor ao pacote.
  udp_client.endPacket();//Finaliza o pacote e envia.
}

void udpListen() {
   if (udp_server.parsePacket() > 0) {
       String req = "";
       while (udp_server.available() > 0) req += (char) udp_server.read();
       Serial.println(req);
   }
}

void readButton() {
    if(!digitalRead(BUTTON) && not_pressed) {
      Serial.println("Send");
      udpSend("oi");
      not_pressed = false;
    }
    else if(digitalRead(BUTTON)) not_pressed = true;
}

void sender(void *arg) {
    while(1) {
        readButton();
        vTaskDelay(1);
    }
}

void server(void *arg) {
    while(1) {
        udpListen();
        vTaskDelay(1);
    }
}

void WiFiStart() {
  WiFiManager wifiManager;
  wifiManager.autoConnect(NAME);
  Serial.println("Ready");
  Serial.print("IP address Host: ");
  Serial.println(WiFi.localIP());
  Serial.print("IP address Dest: ");
  Serial.println(DEST);
}

void restartBPMCount() {
  beatsPerMinute = 60;
  beatAvg = 60;
  for (int i = 0; i < RATE_SIZE; i++) rates[i] = 60;
}

void sensorStart () {
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  restartBPMCount();
  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
}

void readSensor() {
  long irValue = particleSensor.getIR();
  if (irValue >= 50000){
    if (checkForBeat(irValue) == true) {
      suspend = false;
      xTaskResumeFromISR(playerHandle);
      long delta = millis() - lastBeat;
      lastBeat = millis();
      beatsPerMinute = 60 / (delta / 1000.0);
      if (beatsPerMinute < 255 && beatsPerMinute > 20) {
        rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
        rateSpot %= RATE_SIZE; //Wrap variable
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE ; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }
    Serial.print ("rate=");
    Serial.print((1E6*60)/(SR*beatAvg));
    Serial.print(", IR=");
    Serial.print(irValue);
    Serial.print(", BPM=");
    Serial.print(beatsPerMinute);
    Serial.print(", Avg BPM=");
    Serial.println(beatAvg);
  }
  else {
    restartBPMCount();
    suspend = true;
    vTaskSuspend(playerHandle);
  }
}

void getSensor(void *arg) {
    while(1) {
        readSensor();
        vTaskDelay(1);
    }
}


void taskCreator() {
  xTaskCreate(sender,
                      "sender",
                      2048,
                      NULL,
                      4,
                      NULL);

  xTaskCreate(server,
                      "server",
                      2048,
                      NULL,
                      8,
                      NULL);

  xTaskCreate(getSensor,
                      "sensor",
                      2048,
                      NULL,
                      1,
                      NULL);
  xTaskCreate(beatPlayer,
                      "player",
                      2048,
                      NULL,
                      2,
                      & playerHandle);
}

void setup(){
    Serial.begin(115200);
    Serial.println("Booting");
    pinMode(LED, OUTPUT);
    pinMode(BUTTON, INPUT_PULLUP);
    WiFiStart();
    ArduinoOTA.setHostname(NAME);
    ArduinoOTA.begin();
    udp_server.begin(PORT);
    sensorStart();
    taskCreator();
}

void loop(){
    ArduinoOTA.handle();
}
