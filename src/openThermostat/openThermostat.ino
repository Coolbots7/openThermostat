#include <DHT.h>

// ====== DHT11 Settings ======
#define DHT_PIN 0

#define DHT_TYPE DHT11   // DHT 11
//#define DHT_TYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHT_TYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHT_PIN, DHT_TYPE);


void setup() {
  Serial.begin(115200);

  // Initialize device.
  dht.begin();
}

void loop() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
  }
  else {
    Serial.print("Temp: ");
    Serial.print(t);
    Serial.print("°C    Hum: ");
    Serial.print(h);
    Serial.println("%");
  }

  // Delay between measurements.
  delay(2000);
}
