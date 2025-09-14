#include <WiFi.h>
#include <DHT.h>
#include <WiFiCredentials.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <Wire.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define FAN_RELAY_PIN 26
#define LED_RELAY_PIN 27  

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

DHT dht(DHTPIN, DHTTYPE);

WiFiServer server(80);
 
bool isFanRelayOn() {
  return digitalRead(FAN_RELAY_PIN) == LOW;
}

bool isLedRelayOn() {
  return digitalRead(LED_RELAY_PIN) == LOW; 
}

// Terminal-style line output
int currentLine = 0;
const int lineHeight = 9; // for font size 1
String printerName = "CR 10 SE";
String moduleName = "Creality cr 10 se";

void printToOLED(String msg) {
  u8g2.setDrawColor(1);
  u8g2.setFont(u8g2_font_profont11_mr);	
  u8g2.setCursor(0, currentLine * lineHeight + 9); // Set cursor position
  currentLine++;
  String line = "> " + msg;
  u8g2.print(line.c_str());	
  u8g2.sendBuffer();					
  delay(1000);
  if (currentLine * lineHeight >= 64) {
    delay(1000); // Optional pause before clearing
    u8g2.clearBuffer();
    currentLine = 0;
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.begin(115200);
  delay(50);

  // Ensure ESP32 uses the right I2C pins for the OLED
  Wire.begin(21, 22);      // SDA=21, SCL=22
  Wire.setClock(100000);   // safer for some modules; try 400000 if stable

  u8g2.begin();
  u8g2.enableUTF8Print();
  delay(500);

  // Terminal-style OLED prints
  printToOLED("Hello from ESP32!");
  printToOLED("Connecting WiFi...");

  Serial.println("Connecting to WiFi...");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  printToOLED("WiFi connected.");
  printToOLED("IP:"+ WiFi.localIP().toString());

  Serial.println("\nWiFi connected. IP address:");
  Serial.println(WiFi.localIP());

  server.begin();
  pinMode(FAN_RELAY_PIN, OUTPUT);
  pinMode(LED_RELAY_PIN, OUTPUT);
  digitalWrite(FAN_RELAY_PIN, HIGH);
  digitalWrite(LED_RELAY_PIN, HIGH);
  delay(3000);
}

void loop() {
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client connected");

    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n' && request.endsWith("\r\n\r\n")) {
          break;
        }
      }
    }

    // Parse fan relay command
    if (request.indexOf("GET /fan-relay?on=true") >= 0) {
      digitalWrite(FAN_RELAY_PIN, LOW);
      Serial.println("Fan Relay ON");
    } else if (request.indexOf("GET /fan-relay?on=false") >= 0) {
      digitalWrite(FAN_RELAY_PIN, HIGH);
      Serial.println("Fan Relay OFF");
    }

    // Parse led relay command
    if (request.indexOf("GET /led-relay?on=true") >= 0) {
      digitalWrite(LED_RELAY_PIN, LOW);
      Serial.println("LED Relay ON");
    } else if (request.indexOf("GET /led-relay?on=false") >= 0) {
      digitalWrite(LED_RELAY_PIN, HIGH);
      Serial.println("LED Relay OFF");
    }

    // Serve JSON response
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json; charset=utf-8\r\n");
    client.printf(
      "{\"temperature\": %.1f, \"humidity\": %.1f, \"fanStatus\": %s, \"ledStatus\": %s, \"moduleName\": \"%s\"}\n",
      temp, hum, isFanRelayOn() ? "true" : "false", isLedRelayOn() ? "true" : "false", moduleName.c_str()
    );
    client.flush();
    client.stop();
    Serial.println("Client disconnected");
  }

  // Update oled
  // --- U8g2 equivalent of your text UI ---
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);                  // "white" on black

  // 11px
  u8g2.setFont(u8g2_font_t0_18b_mf);      // bigger font for header
  u8g2.setCursor(0, 11);                 // y is baseline, not top-left
  u8g2.print(printerName);

  // Body lines
  u8g2.setFont(u8g2_font_7x14_mf);       // medium font for values
  int y = 24;
  u8g2.setCursor(0, y);  u8g2.printf("Temp: %.1f", temp); 
  u8g2.print("°C");                         // needs enableUTF8Print(); see setup
  y += 13;

  u8g2.setCursor(0, y);  u8g2.printf("Hum:  %.1f%%", hum); 
  y += 13;

  u8g2.setCursor(0, y);  u8g2.printf("Fan:  %s", isFanRelayOn() ? "ON" : "OFF");
  y += 13;

  u8g2.setCursor(0, y);  u8g2.printf("LED:  %s", isLedRelayOn() ? "ON" : "OFF");

  u8g2.sendBuffer();                      // push to screen

  delay(1000);  // Refresh every second
}

