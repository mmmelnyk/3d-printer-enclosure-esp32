#include <WiFi.h>
#include <DHT.h>
#include <WiFiCredentials.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define RELAY_PIN 16  // Pin for relay control

DHT dht(DHTPIN, DHTTYPE);
// oled
#define OLED_DC    17
#define OLED_RESET 2
#define OLED_CS    -1  // or -1 if tied to GND

Adafruit_SSD1306 display(128, 64, &SPI, OLED_DC, OLED_RESET, OLED_CS);

WiFiServer server(80);

String getRelayStatus() {
  return digitalRead(RELAY_PIN) == HIGH ? "ON" : "OFF";
}

// Terminal-style line output
int currentLine = 0;
const int lineHeight = 8; // for font size 1

void printToOLED(String msg) {
  display.setCursor(0, currentLine * lineHeight);
  display.println("> " + msg);
  display.display();
  currentLine++;

  if (currentLine * lineHeight >= 64) {
    delay(1000); // Optional pause before clearing
    display.clearDisplay();
    currentLine = 0;
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  // oled start
  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("SSD1306 allocation failed");
    for (;;); // halt
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

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
  printToOLED("IP:");
  printToOLED(WiFi.localIP().toString());

  Serial.println("\nWiFi connected. IP address:");
  Serial.println(WiFi.localIP());

  server.begin();
  pinMode(RELAY_PIN, OUTPUT);
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

    // Parse relay command
    if (request.indexOf("GET /relay?state=on") >= 0) {
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("Relay ON");
    } else if (request.indexOf("GET /relay?state=off") >= 0) {
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("Relay OFF");
    }

    // Serve web page
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html\n");
    client.println("<!DOCTYPE html><html><head><meta charset='utf-8'><title>ESP32 Control</title></head><body>");
    client.printf("<h1>Temperature: %.1f &deg;C</h1>", temp);
    client.printf("<h2>Humidity: %.1f %%</h2>", hum);
    client.printf("<p><strong>Fan Status:</strong> %s</p>", getRelayStatus().c_str());
    client.println("<p><a href='/relay?state=on'><button>Turn ON Relay</button></a></p>");
    client.println("<p><a href='/relay?state=off'><button>Turn OFF Relay</button></a></p>");
    client.println("</body></html>");

    client.flush();
    client.stop();
    Serial.println("Client disconnected");
  }

  // Update oled
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.printf("CR 10 SE\n");
  display.printf("Temp:%.1fC\n", temp);
  display.printf("Hum:%.1f%%\n", hum);
  display.printf("Fan: %s", getRelayStatus().c_str());
  display.display();
  
  delay(1000);  // Refresh every second
}

