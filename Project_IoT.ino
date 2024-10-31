//BLYNK
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME "Health Monitoring"
#define BLYNK_AUTH_TOKEN ""

#define BLYNK_PRINT Serial
#include <Blynk.h>
#include <BlynkSimpleEsp32.h>
#include <WiFi.h>
#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include <DHT.h>
#include <Firebase_ESP_Client.h>

// Firebase configuration
#define FIREBASE_HOST ""
#define FIREBASE_AUTH ""

// Firebase objects
FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;

// MAX30100 Pulse Oximeter object
PulseOximeter pox;
BlynkTimer timer;
// DHT11 Sensor setup
#define DHTPIN 14              // Pin where the DHT11 is connected
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
//buzzer
const int buzzer = 12; //D6
// WiFi credentials
const char* ssid = "ThuHa";
const char* password = "thuha1707";

// Variables for data
float heartRate, spo2, t, h;
float lastBPM,lastspo2,lastt,lasth;
float BPM1=0;
float SpO21=0;
unsigned long tsLastReport = 0;
unsigned long tsLastDHTReport = 0;
const unsigned long firebaseSendInterval = 1000;
unsigned long lastFirebaseSendTime = 0;
unsigned long lastblynkSendTime =0;

void myTimerEvent()
{
// You can send any value at any time.
// Please don't send more that 10 values per second.
Blynk.virtualWrite(V0, millis() / 1000);
}

void onBeatDetected() {
  Serial.println("♥ Beat!");
}

void setup() {
  Serial.begin(115200);
  pinMode(buzzer, OUTPUT); 
  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  timer.setInterval(1000L, myTimerEvent);
  // Initialize MAX30100 Pulse Oximeter
  if (!pox.begin()) {
    Serial.println("Failed to initialize MAX30100 sensor.");
    while (1);
  }
  Serial.println("MAX30100 initialized!");
  pox.setOnBeatDetectedCallback(onBeatDetected);
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  // Initialize DHT11
  dht.begin();
  Serial.println("DHT11 initialized!");
  // Firebase configuration
  firebaseConfig.database_url = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;
  // Initialize Firebase
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);
  // Create tasks for dual-task operation
  xTaskCreatePinnedToCore(readSensors, "Sensor Task", 4096, NULL, 1, NULL, 0); // Run on core 0
  xTaskCreatePinnedToCore(sendDataToFirebase, "Firebase Task", 8192, NULL, 1, NULL, 1); // Increased stack size to 8192 and run on core 1
}

// Task 1: Reads data from sensors every 2 seconds
void readSensors(void *parameter) {
  for (;;) {
    pox.update();
    // Update the heart rate and SpO2 every 1 seconds
    if (millis() - tsLastReport > 3000) {
      BPM1 = pox.getHeartRate();
      SpO21 = pox.getSpO2();
      
      if (BPM1 > 0 && SpO21 > 0) {
        Serial.print("Heart rate: ");
        Serial.print(BPM1);
        Serial.print(" bpm, SpO2: ");
        Serial.print(SpO21);
        Serial.println(" %");
      } else {
        Serial.println("Invalid sensor readings.");
      }
      tsLastReport = millis();
    }

    if (millis() - tsLastDHTReport > 2000) {
      // Read temperature and humidity from the DHT11 sensor every 1 seconds
      t = dht.readTemperature();
      h = dht.readHumidity();
      if (isnan(t) || isnan(h)) {
        Serial.println("Failed to read from DHT sensor!");
      } else {
        Serial.print("Temperature: ");
        Serial.print(t);
        Serial.print(" °C, Humidity: ");
        Serial.print(h);
        Serial.println(" %");
      }
      tsLastDHTReport = millis();
      }
  }
}

// Task 2: Pushes data to Firebase every 5 seconds
void sendDataToFirebase(void *parameter) {
  for (;;) {
    Blynk.run();
    timer.run();
    heartRate=BPM1;
    spo2=SpO21;
    float humi2 = h;
    float temp2 = t;
    if (millis() - lastFirebaseSendTime > firebaseSendInterval) {
      // Check if valid data is available for each sensor
      if (heartRate > 0 && spo2 > 0) {
        if (Firebase.RTDB.setFloat(&firebaseData, "PhuongsensorData/heartRate", heartRate)) {
          Serial.println("Heart rate data pushed to Firebase successfully!");
        } else {
          Serial.print("Failed to push heart rate data: ");
          Serial.println(firebaseData.errorReason());
        }

        if (Firebase.RTDB.setFloat(&firebaseData, "PhuongsensorData/spo2", spo2)) {
          Serial.println("SpO2 data pushed to Firebase successfully!");
        } else {
          Serial.print("Failed to push SpO2 data: ");
          Serial.println(firebaseData.errorReason());
        }
      }

      // Push temperature and humidity to Firebase if available

      if (!isnan(temp2) && !isnan(humi2)) {
        if (Firebase.RTDB.setFloat(&firebaseData, "PhuongDHTdata/temperature", temp2)) {
          Serial.println("Temperature data pushed to Firebase successfully!");
        } else {
          Serial.print("Failed to push temperature data: ");
          Serial.println(firebaseData.errorReason());
        }
        if (Firebase.RTDB.setFloat(&firebaseData, "PhuongDHTdata/humidity", humi2)) {
          Serial.println("Humidity data pushed to Firebase successfully!");
        } else {
          Serial.print("Failed to push humidity data: ");
          Serial.println(firebaseData.errorReason());
        }
      }
      lastFirebaseSendTime = millis();
    }
    if (millis() - lastblynkSendTime > 2000)
    {
      if (heartRate>0&& spo2>0)
      {
        if (heartRate!=lastBPM) 
          {
            Blynk.virtualWrite(V1, heartRate);
            lastBPM=heartRate;
          }
          delay(100);
          if (spo2!=lastspo2) 
          {
            Blynk.virtualWrite(V2, spo2);
            lastspo2=spo2;
          }
          delay(100);
        if (temp2!=lastt) 
        {
          Blynk.virtualWrite(V3, temp2);
          lastt=temp2;
        }
        delay(100);
        if (humi2!=lasth) 
        {
          Blynk.virtualWrite(V4, humi2);
          lasth=humi2;
        }
        delay(100);
      }
      lastblynkSendTime=millis();
    }
    if ((heartRate<60 || heartRate>100 || spo2<95 || temp2>37 || temp2<16 || humi2<30 || humi2>70) && (heartRate>0 && spo2>0 && temp2 >0 && humi2>0))
    {
      digitalWrite(buzzer,HIGH);///*
      static unsigned long lastAlertTime = 0;
      if (millis() - lastAlertTime > 5000) 
      {
      //canh bao nhip tim
      if(heartRate<60) Blynk.logEvent("slow_heart_rate", String("Slow heart rate: ") + heartRate);
      delay(100);
      if (heartRate>100) Blynk.logEvent("high_heart_rate", String("High heart rate: ") + heartRate);
      delay(100);
      //canh bao SpO2
      if(spo2<95) Blynk.logEvent("low_spo2", String("Low SpO2: ") + spo2);
      delay(100);
      //canh bao nhiet do
      if(temp2>37 || temp2<15) Blynk.logEvent("dangerous_temperature", String("Dangerous TemperatureDetected!: ") + temp2);
      delay(100);
      // canh bao do am
      if (humi2>70 || humi2<30) Blynk.logEvent("dangerous_humidity", String("Dangerous Humidity: ") + humi2);
      delay(100);
      //*/
      lastAlertTime = millis();
      }
    }
    else
    {
      digitalWrite(buzzer,LOW);
    }
  }
}

void loop() {
  // Main loop does nothing. Tasks handle all operations.
}
