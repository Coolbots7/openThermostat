#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <DHT.h>


// ====== Screen Settings ======
#define SCREEN_ADDR 0x3C

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ====== Relay Settings ======
#define FURNACE_RELAY_PIN 16

// ====== Temperature Sensor Settings ======
#define DHT_PIN 0

#define DHT_TYPE DHT11   // DHT 11
//#define DHT_TYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHT_TYPE DHT21   // DHT 21 (AM2301)

DHT dht(DHT_PIN, DHT_TYPE);


void setup() {
  Serial.begin(115200);

  // Initialize relays
  pinMode(FURNACE_RELAY_PIN, OUTPUT);
  digitalWrite(FURNACE_RELAY_PIN, LOW);


  // Initialize screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }

  // Clear the screen buffer
  display.clearDisplay();

  // Update screen
  display.display();


  // Initialize temperature sensor.
  // TODO show DHT initialization on screen
  dht.begin();
}

void loop() {
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();

  // Clear the screen buffer
  display.clearDisplay();
  // Draw 2X-scale text
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));

    display.setCursor(0, 0);
    display.print(F("Failed to read from DHT sensor!"));
  }
  else {
    Serial.print("Temp: ");
    Serial.print(t);
    Serial.print("°C    Hum: ");
    Serial.print(h);
    Serial.println("%");

    display.setCursor(0, 0);
    display.print(F("Temp: "));
    display.print(t);
    display.print("C");

    display.setCursor(0, 20);
    display.print(F("Humidity: "));
    display.print(h);
    display.print("%");
  }

  // Toggle relay
  digitalWrite(FURNACE_RELAY_PIN, !digitalRead(FURNACE_RELAY_PIN));

  display.setCursor(0, 40);
  display.print(F("Heat: "));
  display.print(digitalRead(FURNACE_RELAY_PIN) == HIGH ? "On" : "Off");

  // Update screen
  display.display();

  // Delay between measurements.
  delay(2000);
}
