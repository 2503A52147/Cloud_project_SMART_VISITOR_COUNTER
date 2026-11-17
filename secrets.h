
#ifndef SECRETS_H
#define SECRETS_H

// Wi-Fi credentials
#define WIFI_SSID "your wifi name"
#define WIFI_PASSWORD "your wifi password"

// AWS IoT endpoint: hostname only, no https://
#define AWS_IOT_ENDPOINT "replace your aws end point here"

// AWS Root CA certificate
static const char AWS_CERT_CA[] PROGMEM = R"EOF(
paste your amazon root CA certificate here
)EOF";

// ESP32 device certificate
static const char AWS_CERT_CRT[] PROGMEM = R"EOF(
paste your device certificate here
)EOF";

// ESP32 private key
static const char AWS_CERT_PRIVATE[] PROGMEM = R"EOF(
paste your private key here
)EOF";

#endif
