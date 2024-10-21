#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include "DHT.h"
#define BLYNK_PRINT Serial
#include <Blynk.h>
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#define REPORTING_PERIOD_MS     1000  //khoang thoi gian giua cac lan do: 1s
#define DHT_PERIOD_MS           1000  // DHT11 requires a 1-2s interval between readings
char ssid[] = "";
char pass[] = "";
float BPM,SpO2;

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
const int DHTPIN = 14; //D5
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

void setup() {
  Serial.begin(115200);
  pinMode(buzzer, OUTPUT); 
  dht.begin();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  timer.setInterval(1000L, myTimerEvent);
  Serial.print("Initializing pulse oximeter...");
  // Initialize MAX30100 sensor
  if (!pox.begin()) //khởi tạo MAX30100 ko thanh cong (pox.begin la ma khoi tao MAX30100)
  {
    Serial.println("FAILED");
    for (;;); //vong lap vo han -->phai reset lai chuong trinh 
  } else  // khoi tao thanh cong
  {
    Serial.println("SUCCESS");
  }
  
  // Configure MAX30100 sensor LED current
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  // Register callback for heartbeat detection
  pox.setOnBeatDetectedCallback(onBeatDetected); //goi ham onBeatDetected khi phát hiện nhịp tim
}

void loop() {
  // Update MAX30100 sensor readings
  pox.update();
  Blynk.run();
  timer.run();
  // Report Heart Rate and SpO2 every 1 second
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) // kiem tra du thoi gian giua cac khoang do
  {
    BPM = pox.getHeartRate();
    SpO2 = pox.getSpO2();
    Serial.print("Heart rate: ");
    Serial.print(pox.getHeartRate());
    Serial.print(" bpm / SpO2: ");
    Serial.print(pox.getSpO2());
    Serial.println(" %");
    if (BPM > 0) 
    {
      Blynk.virtualWrite(V1, BPM);
    }
    if (SpO2 > 0) 
    {
      Blynk.virtualWrite(V2, SpO2);
    }

    tsLastReport = millis();
    tsLastReport = millis();
  }

  // Read and report DHT11 sensor data every 1 seconds
  if (millis() - tsLastDHTReport > DHT_PERIOD_MS) 
  {
    float h = dht.readHumidity();    // Read humidity
    float t = dht.readTemperature(); // Read temperature in degrees Celsius
    float f = dht.readTemperature(true); // Read temperature in Fahrenheit
        
    if (isnan(h) || isnan(t) || isnan(f)) 
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
      Serial.print(" *C ");
    }

    tsLastDHTReport = millis();
  }
  if (BPM<50 || BPM>150 || SpO2<90)
  {
      digitalWrite(buzzer,HIGH);
  }
  else
  {
    digitalWrite(buzzer,LOW);
  }
}