//BLYNK
#define BLYNK_TEMPLATE_ID "TMPL6joXrYP2B"
#define BLYNK_TEMPLATE_NAME "Health Monitoring"
#define BLYNK_AUTH_TOKEN "RGq1r_vpLbfqZ0tA-l5GR1bBHk9_Mujl"
#define BLYNK_PRINT Serial
#include <Blynk.h>

#include <Wire.h>

//Sensors
#include "MAX30100_PulseOximeter.h"
#include "DHT.h"

//For ESP8266
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

//For ESP32
//#include <WiFi.h>
//#include <WiFiClient.h>
//#include <BlynkSimpleEsp32.h>

//Firebase
#include <Firebase_ESP_Client.h>
// Firebase configuration
#define FIREBASE_HOST "https://healthmonitoringha-default-rtdb.asia-southeast1.firebasedatabase.app" // Replace with your Firebase database URL
#define FIREBASE_AUTH "AIzaSyBV7YLpstv5r8CbdRD8NBVtLpXsC1m2CSM"      // Replace with your Firebase secret

// Firebase objects
FirebaseData firebaseData;
FirebaseConfig firebaseConfig;
FirebaseAuth firebaseAuth;
#define REPORTING_PERIOD_MS     2000  //khoang thoi gian giua cac lan do: 1s
#define DHT_PERIOD_MS           3000  // DHT11 requires a 1-2s interval between readings

//Wifi
char ssid[] = "ThuHa";
char pass[] = "thuha1707";

float BPM,SpO2;
float h,t,f;
float lastBPM = 0;
float lastSpO2 = 0;

void myTimerEvent()
{
  // You can send any value at any time.
  // Please don't send more that 10 values per second.
  Blynk.virtualWrite(V0, millis() / 1000);
}

// Create a PulseOximeter object
// Connections : SCL PIN - D1 , SDA PIN - D2 , INT PIN - D0
PulseOximeter pox; //Khởi tạo đối tượng PulseOximeter

// DHT11
const int DHTPIN = 14;
const int DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);

//buzzer
const int buzzer = 12; //D6

// Variables for timing
uint32_t tsLastReport = 0; //Biến để theo dõi thời gian báo cáo
uint32_t tsLastDHTReport = 0;
BlynkTimer timer;

// Callback routine executed when a pulse is detected
void onBeatDetected() 
{
    Serial.println("♥ Beat!");
}

void setup() 
{
  Serial.begin(115200);
  pinMode(buzzer, OUTPUT); 
  
  // Initialize WiFi
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  //DHT
  dht.begin();

  //BLYNK
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(1000L, myTimerEvent);

  //Max30100
  Serial.print("Initializing pulse oximeter...");
  // Initialize MAX30100 sensor
  if (!pox.begin()) //khởi tạo MAX30100 ko thanh cong (pox.begin la ma khoi tao MAX30100)
  {
    Serial.println("Failed to initialize MAX30100 sensor.");
    while (1); //vong lap vo han -->phai reset lai chuong trinh 
  } else  // khoi tao thanh cong
  {
    Serial.println("MAX30100 initialized!");
  }
  
  // Configure MAX30100 sensor LED current
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  // Register callback for heartbeat detection
  pox.setOnBeatDetectedCallback(onBeatDetected); //goi ham onBeatDetected khi phát hiện nhịp tim
  // Firebase configuration
  firebaseConfig.database_url = FIREBASE_HOST;
  firebaseConfig.signer.tokens.legacy_token = FIREBASE_AUTH;  // Use Firebase Database Secret

  // Initialize Firebase
  Firebase.begin(&firebaseConfig, &firebaseAuth);
  Firebase.reconnectWiFi(true);  // Automatically reconnect WiFi when disconnected
}

void loop() 
{
    // Update MAX30100 sensor readings
    pox.update();

    //Blynk
    Blynk.run();
    timer.run();

    // Report Heart Rate and SpO2 every 1 second
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) // kiem tra du thoi gian giua cac khoang do
    {
      BPM = pox.getHeartRate();
      SpO2 = pox.getSpO2();
      if (BPM>0 && SpO2>0)
      {
        Serial.print("Heart rate: ");
        Serial.print(BPM);
        Serial.print(" bpm / SpO2: ");
        Serial.print(SpO2);
        Serial.println(" %");

        Blynk.virtualWrite(V1, BPM);
        Blynk.virtualWrite(V2, SpO2);

        // Push heart rate to Firebase
        if (Firebase.RTDB.setFloat(&firebaseData, "Maxdata/heartrate", BPM)) 
        {
          Serial.println("Heart rate data pushed to Firebase successfully!");
        } else 
        {
          Serial.print("Failed to push heart rate data: ");
          Serial.println(firebaseData.errorReason());
        }
        
        // Push SpO2 to Firebase
        if (Firebase.RTDB.setFloat(&firebaseData, "Maxdata/spo2", SpO2)) 
        {
          Serial.println("SpO2 data pushed to Firebase successfully!");
        } else 
        {
          Serial.print("Failed to push SpO2 data: ");
          Serial.println(firebaseData.errorReason());
        }
      } else 
      {
        Serial.println("No valid readings from MAX30100.");
      }
      tsLastReport = millis();
    }

    // Read and report DHT11 sensor data every 2 seconds
    if (millis() - tsLastDHTReport > DHT_PERIOD_MS) 
    {
      h = dht.readHumidity();    // Read humidity
      t = dht.readTemperature(); // Read temperature in degrees Celsius
      if (isnan(t) || isnan(h)) 
      {
        Serial.println("Failed to read from DHT sensor!");
      } 
      else 
      {
        Serial.print("Humidity: ");
        Serial.print(h);
        Serial.print(" %\t");
        Serial.print("Temperature: ");
        Serial.print(t);
        Serial.println(" *C ");

        //BLYNK
        Blynk.virtualWrite(V3, t);
        Blynk.virtualWrite(V4, h);
        
        // Push temperature to Firebase
        if (Firebase.RTDB.setFloat(&firebaseData, "DHTdata/temperature", t)) 
        {
          Serial.println("Temperature data pushed to Firebase successfully!");
        } else 
        {
          Serial.print("Failed to push temperature data: ");
          Serial.println(firebaseData.errorReason());
        }
        // Push humidity to Firebase
        if (Firebase.RTDB.setFloat(&firebaseData, "DHTdata/humidity", h)) 
        {
          Serial.println("Humidity data pushed to Firebase successfully!");
        } else 
        {
          Serial.print("Failed to push humidity data: ");
          Serial.println(firebaseData.errorReason());
        }
      }

      tsLastDHTReport = millis();
    }

    // canh bao
    if ((BPM<60 || BPM>100 || SpO2<95 || t>37 || t<16 || h<30 || h>70) && (BPM>0 && SpO2>0 && t >0 && h>0))
    {
      digitalWrite(buzzer,HIGH);///*
      static unsigned long lastAlertTime = 0;
      if (millis() - lastAlertTime > 5000) 
      {
      //canh bao nhip tim
      if(BPM<60) Blynk.logEvent("slow_heart_rate", String("Slow heart rate: ") + BPM);
      if (BPM>100) Blynk.logEvent("high_heart_rate", String("High heart rate: ") + BPM);
      //canh bao SpO2
      if(SpO2<95) Blynk.logEvent("low_spo2", String("Low SpO2: ") + SpO2);
      //canh bao nhiet do
      if(t>37 || t<15) Blynk.logEvent("dangerous_temperature", String("Dangerous TemperatureDetected!: ") + t);
      // canh bao do am
      if (h>70 || h<30) Blynk.logEvent("dangerous_humidity", String("Dangerous Humidity: ") + h);
      //*/
      lastAlertTime = millis();
      }
    }
    else
    {
      digitalWrite(buzzer,LOW);
    }
}