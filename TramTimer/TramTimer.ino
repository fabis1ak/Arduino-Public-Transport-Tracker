#include <WiFiS3.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

// ===== WIFI =====
const char* ssid = "wifi_name";
const char* password = "password";

// ===== ZTM API =====
const char* host = "api.um.warszawa.pl";
String apiKey = "api_key";

// ===== LCD =====
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== PRZYSTANEK =====
// from → to
String stopId = "";
String stopNr = "";

WiFiClient client;

void setup() {
  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Connecting wifi");

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  lcd.clear();
  lcd.print("WiFi OK");
  delay(1000);
}

void loop() {
  String t1 = "--:--";
  String t2 = "--:--";

  if (getDepartures(t1, t2)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("00->END");

    lcd.setCursor(0, 1);
    lcd.print(t1);
    lcd.print(" ");
    lcd.print(t2);
  } else {
    lcd.clear();
    lcd.print("No data");
  }

  delay(30000);  // odswiezanie co 30s
}

bool getDepartures(String &t1, String &t2) {
  if (!client.connect(host, 80)) return false;

  String url = " ";
  url += "&busstopId=" + stopId;
  url += "&busstopNr=" + stopNr;
  url += "&apikey=" + apiKey;

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  while (client.connected() && !client.available()) delay(10);

  String response;
  while (client.available()) response += client.readString();
  client.stop();

  int jsonStart = response.indexOf('{');
  if (jsonStart < 0) return false;

  String json = response.substring(jsonStart);

  StaticJsonDocument<8192> doc;
  if (deserializeJson(doc, json)) return false;

  JsonArray arr = doc["result"];

  int found = 0;
  for (JsonObject obj : arr) {
    if (obj["values"][0]["value"] == "22") {
      String time = obj["values"][5]["value"];

      if (found == 0) t1 = time;
      if (found == 1) {
        t2 = time;
        break;
      }
      found++;
    }
  }

  return (found > 0);
}
