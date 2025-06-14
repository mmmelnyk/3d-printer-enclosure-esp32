#include <WiFi.h>
#include <DHT.h>

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.println("Connecting to WiFi...");
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP address:");
  Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Client connected");

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("DHT read failed");
      client.println("HTTP/1.1 503 Service Unavailable");
      client.println("Content-type:text/plain\n");
      client.println("Sensor read error");
      client.stop();
      return;
    }

    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html\n");
    client.println("<!DOCTYPE html><html><head><meta charset='utf-8'><title>ESP32 Sensor</title></head><body>");
    client.printf("<h1>Temperature: %.1f &deg;C</h1>", temp);
    client.printf("<h2>Humidity: %.1f %%</h2>", hum);
    client.println("</body></html>");

    delay(10);  // allow client to receive response
    client.stop();
    Serial.println("Client disconnected");
  }
}
