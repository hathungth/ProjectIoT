#include <Wire.h>
#include "MAX30100_PulseOximeter.h"
#include "DHT.h"

#define REPORTING_PERIOD_MS     1000
#define DHT_PERIOD_MS           2000  // DHT11 requires a 1-2s interval between readings

// Create a PulseOximeter object
PulseOximeter pox;

// DHT11
const int DHTPIN = 14;
const int DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);

//buzzer
const int buzzer = 12;

// Variables for timing
uint32_t tsLastReport = 0;
uint32_t tsLastDHTReport = 0;

// Callback routine executed when a pulse is detected
void onBeatDetected() 
{
    Serial.println("♥ Beat!");
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.print("Initializing pulse oximeter...");
  // Initialize MAX30100 sensor
  if (!pox.begin()) 
  {
    Serial.println("FAILED");
    for (;;);
  } else 
  {
    Serial.println("SUCCESS");
  }
  
  // Configure MAX30100 sensor LED current
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  // Register callback for heartbeat detection
  pox.setOnBeatDetectedCallback(onBeatDetected);
}

void loop() {
    // Update MAX30100 sensor readings
    pox.update();

    // Report Heart Rate and SpO2 every 1 second
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) 
    {
        Serial.print("Heart rate: ");
        Serial.print(pox.getHeartRate());
        Serial.print(" bpm / SpO2: ");
        Serial.print(pox.getSpO2());
        Serial.println(" %");
        tsLastReport = millis();
    }

    // Read and report DHT11 sensor data every 2 seconds
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
}
