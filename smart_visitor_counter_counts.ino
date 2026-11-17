
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"

// OLED configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

// Hardware pins
#define BUTTON_PIN 4

// MQTT topic
const char* MQTT_TOPIC = "visitor/count";

// Button timing
const unsigned long debounceDelay = 50;
const unsigned long longPressDuration = 1000;

// OLED
Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// Secure MQTT connection
WiFiClientSecure secureClient;
PubSubClient mqtt(secureClient);

// Total visitor counter
int visitorCount = 0;

// Visitor categories
const char* categories[] = {
  "Children",
  "Men",
  "Women"
};

const int totalCategories = 3;
int selectedCategory = 0;

// Button debounce variables
int lastButtonReading = HIGH;
int stableButtonState = HIGH;

unsigned long lastDebounceTime = 0;
unsigned long pressStartTime = 0;

bool longPressHandled = false;

// Display information
void updateDisplay(String status) {

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  // Heading
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("SMART VISITOR COUNTER");

  display.drawLine(0, 12, 128, 12, SSD1306_WHITE);

  // Selected category
  display.setTextSize(1);
  display.setCursor(0, 17);
  display.println("Category:");

  display.setTextSize(2);
  display.setCursor(0, 27);
  display.println(categories[selectedCategory]);

  // Total visitors
  display.setTextSize(1);
  display.setCursor(0, 48);
  display.print("Total: ");
  display.println(visitorCount);

  // Status
  display.setTextSize(1);
  display.setCursor(0, 57);
  display.println(status);

  display.display();
}

// Connect to Wi-Fi
void connectWiFi() {

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");
  Serial.println(WiFi.localIP());
}

// Connect to AWS IoT Core
void connectAWS() {

  secureClient.setCACert(AWS_CERT_CA);
  secureClient.setCertificate(AWS_CERT_CRT);
  secureClient.setPrivateKey(AWS_CERT_PRIVATE);

  mqtt.setServer(AWS_IOT_ENDPOINT, 8883);
  mqtt.setBufferSize(256);

  while (!mqtt.connected()) {

    Serial.println("Connecting to AWS IoT Core...");

    if (mqtt.connect("SmartVisitorCounter")) {

      Serial.println("AWS MQTT Connected!");

    } else {

      Serial.print("MQTT connection failed. State: ");
      Serial.println(mqtt.state());

      delay(3000);
    }
  }
}

// Publish visitor category to AWS
void publishVisitorCount() {

  if (!mqtt.connected()) {
    connectAWS();
  }

  char payload[200];

  snprintf(
    payload,
    sizeof(payload),
    "{\"device\":\"SmartVisitorCounter\","
    "\"visitor_count\":%d,"
    "\"category\":\"%s\"}",
    visitorCount,
    categories[selectedCategory]
  );

  Serial.println();
  Serial.println("Publishing visitor information:");

  Serial.println(payload);

  if (mqtt.publish(MQTT_TOPIC, payload)) {

    Serial.println("Visitor information sent to AWS!");

    updateDisplay("AWS: SENT");

  } else {

    Serial.println("MQTT Publish Failed!");

    updateDisplay("AWS: FAILED");
  }
}

// Confirm selected visitor category
void confirmCategory() {

  visitorCount++;

  Serial.println();
  Serial.println("VISITOR CONFIRMED");

  Serial.print("Category: ");
  Serial.println(categories[selectedCategory]);

  Serial.print("Total Visitors: ");
  Serial.println(visitorCount);

  updateDisplay("COUNT UPDATED");

  publishVisitorCount();
}

// Setup
void setup() {

  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(21, 22);

  if (!display.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {

    Serial.println("OLED initialization failed!");

    while (true) {
      delay(1000);
    }
  }

  updateDisplay("STARTING...");

  connectWiFi();

  connectAWS();

  updateDisplay("AWS: CONNECTED");

  Serial.println();
  Serial.println("Smart Visitor Counter Ready!");
  Serial.println("Short press: Change category");
  Serial.println("Hold 1 second: Confirm visitor");
}

// Main loop
void loop() {

  // Maintain Wi-Fi connection
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  // Maintain MQTT connection
  if (!mqtt.connected()) {
    connectAWS();
  }

  mqtt.loop();

  // Read button
  int reading = digitalRead(BUTTON_PIN);

  // Detect button reading change
  if (reading != lastButtonReading) {
    lastDebounceTime = millis();
  }

  // Debounce button
  if ((millis() - lastDebounceTime) > debounceDelay) {

    if (reading != stableButtonState) {

      stableButtonState = reading;

      // Button pressed
      if (stableButtonState == LOW) {

        pressStartTime = millis();

        longPressHandled = false;
      }

      // Button released
      else {

        // Short press: change category
        if (!longPressHandled) {

          selectedCategory++;

          if (selectedCategory >= totalCategories) {
            selectedCategory = 0;
          }

          Serial.print("Selected Category: ");
          Serial.println(categories[selectedCategory]);

          updateDisplay("HOLD TO CONFIRM");
        }
      }
    }
  }

  // Detect long press
  if (
    stableButtonState == LOW &&
    !longPressHandled &&
    (millis() - pressStartTime >= longPressDuration)
  ) {

    longPressHandled = true;

    confirmCategory();
  }

  lastButtonReading = reading;

  delay(5);
}
