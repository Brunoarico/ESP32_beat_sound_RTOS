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
#define ERROR_LED LED_BUILTIN
#define LEDSTRIP_PIN 32


WiFiUDP udp_client;
WiFiUDP udp_server;

MAX30105 particleSensor;

const byte RATE_SIZE = 10; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg = 60;
boolean suspend = true;
int received_BPM = 60;

TaskHandle_t playerHandle;

/*--------------------Audio Functions--------------------*/

void beatPlayer(void *arg) {
    while(1) {
    //Serial.println("Start Player");
    for (int i = 0; i < SOUND_SAMPLES; i++){
      dacWrite(DAC_PIN, t[i]);
      ledcWrite(0, t[i]);
      delayMicroseconds((60E6)/(SR*received_BPM));
      if(suspend) break;
    }
    dacWrite(DAC_PIN, 0);
    ledcWrite(0, 0);
    //Serial.println(received_BPM);
    vTaskDelay(((1000*received_BPM)/60.0/2));
  }
}

/*--------------------Comunication Functions--------------------*/

void udpSend(String text) {
  udp_client.beginPacket(DEST, PORT);//Inicializa o pacote de transmissao ao IP e PORTA.
  udp_client.println(text);//Adiciona-se o valor ao pacote.
  udp_client.endPacket();//Finaliza o pacote e envia.
}

void udpListen() {
   if (udp_server.parsePacket() > 0) {
       String req = "";
       while (udp_server.available() > 0) req += (char) udp_server.read();
       //Serial.println(req);
       if(req.toInt() < 0) {
         //Serial.println("STOP");
         received_BPM = 60;
         suspend = true;
         ledcWrite(0, 0);
         vTaskSuspend(playerHandle);
       }
       else if(req.toInt() > 0){
         received_BPM = req.toInt();
         //Serial.println(received_BPM);
         suspend = false;
         xTaskResumeFromISR(playerHandle);
       }
   }
}

void server(void *arg) {
    while(1) {
        udpListen();
        vTaskDelay(1);
    }
}

void reconnect (){
  while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
}

void WiFiStart() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASS);
  Serial.println("Ready");
  reconnect();
  Serial.print("IP address Host: ");
  Serial.println(WiFi.localIP());
  Serial.print("IP address Dest: ");
  Serial.println(DEST);
}

/*--------------------Light Effect Functions--------------------*/

void ledStart(int ledpin) {
  int CH = 0;
  int F = 4000;
  int resolution = 8;
  ledcAttachPin(ledpin, CH);
  ledcSetup(CH, F, resolution);
}

/*--------------------BPM Sensor Functions--------------------*/

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

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x0A);
  particleSensor.setPulseAmplitudeGreen(0);
  restartBPMCount();
  received_BPM = 60;
  suspend = true;
}

void readSensor() {
  long irValue = particleSensor.getIR();
  if (irValue >= 50000) {
    if (checkForBeat(irValue) == true) {
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
    //Serial.println(beatAvg);
    udpSend(String(beatAvg));
  }
  else {
    udpSend("-1");
    vTaskDelay(500);
    restartBPMCount();
  }
}

void getSensor(void *arg) {
    while(1) {
        readSensor();
        vTaskDelay(1);
    }
}

/*--------------------MTasks Manager--------------------*/
void taskCreator() {
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
    delay(1000);
    Serial.println("Booting");
    WiFiStart();
    ArduinoOTA.setHostname(NAME);
    ArduinoOTA.begin();
    udp_server.begin(PORT);
    sensorStart();
    ledStart(LEDSTRIP_PIN);
    taskCreator();
    vTaskSuspend(playerHandle);
}

void loop(){
    ArduinoOTA.handle();
    reconnect();
}
