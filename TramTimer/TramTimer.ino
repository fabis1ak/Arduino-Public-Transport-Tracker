#include <WiFiS3.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiUdp.h>
#include <NTPClient.h>

// ===== WIFI =====
const char* ssid     = "wifi_name";
const char* password = "password";

// ===== ZTM API =====
const char* host   = "api.um.warszawa.pl";
const char* apiKey = "api_key";
String stopId      = "";
String stopNr      = "";

// ===== LINIE =====
// Wpisz numery linii do sledzenia
const int LINE_COUNT = 3;
String lines[]       = {"22", "17", "8"};
int currentLine      = 0;

// ===== PINY =====
const int BUTTON_PIN = 2;   // przycisk GND <-> pin 2
const int BUZZER_PIN = 3;   // buzzer pasywny
const int ALARM_MIN  = 3;   // buzzer gdy odjazd za <= X minut

// ===== STALE =====
const unsigned long REFRESH_MS  = 30000;
const unsigned long TIMEOUT_MS  = 10000;
const int           DEBOUNCE_MS = 50;

// ===== LCD =====
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===== NTP =====
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200); // UTC+2

WiFiSSLClient client;

bool lastBtnState    = HIGH;
unsigned long lastBtnEdge = 0;

// ===== HELPERS =====

void beep(int n) {
  for (int i = 0; i < n; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(100);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < n - 1) delay(100);
  }
}

int timeToMinutes(const String& t) {
  return t.substring(0, 2).toInt() * 60 + t.substring(3, 5).toInt();
}

// Zwraca true jezeli przycisk zostal nacisniety (z debouncingiem)
bool buttonPressed() {
  bool state = digitalRead(BUTTON_PIN);
  if (state == LOW && lastBtnState == HIGH && millis() - lastBtnEdge > DEBOUNCE_MS) {
    lastBtnEdge  = millis();
    lastBtnState = LOW;
    return true;
  }
  if (state == HIGH) lastBtnState = HIGH;
  return false;
}

// ===== API =====

bool getDepartures(const String& line, int& min1, int& min2) {
  if (!client.connect(host, 443)) return false;

  String url = "/api/action/dbtimetable_get/?id=e923fa0e-d96c-43f9-ae6e-60518c9f3238";
  url += "&busstopId=" + stopId;
  url += "&busstopNr=" + stopNr;
  url += "&line=" + line;
  url += "&apikey=" + String(apiKey);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "Connection: close\r\n\r\n");

  // Czekaj na odpowiedz z timeoutem
  unsigned long t = millis();
  while (client.connected() && !client.available()) {
    if (millis() - t > TIMEOUT_MS) { client.stop(); return false; }
    delay(10);
  }

  String body;
  while (client.connected() || client.available()) {
    while (client.available()) body += (char)client.read();
  }
  client.stop();

  int jsonStart = body.indexOf('{');
  if (jsonStart < 0) return false;

  StaticJsonDocument<8192> doc;
  if (deserializeJson(doc, body.c_str() + jsonStart)) return false;

  JsonArray arr = doc["result"];
  if (arr.isNull()) return false;

  int nowMin = timeClient.getHours() * 60 + timeClient.getMinutes();
  int found  = 0;

  for (JsonObject obj : arr) {
    String depTime = obj["values"][5]["value"].as<String>();
    int dep  = timeToMinutes(depTime);
    int diff = dep - nowMin;
    if (diff < -30) diff += 24 * 60; // przejscie przez polnoc
    if (diff < 0 || diff > 120) continue; // pomijaj przeszle i bardzo odlegle

    if (found == 0) min1 = diff;
    else if (found == 1) { min2 = diff; break; }
    found++;
  }

  return found > 0;
}

// ===== SETUP =====

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Laczenie WiFi...");

  WiFi.begin(ssid, password);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries++ < 20) delay(500);

  if (WiFi.status() != WL_CONNECTED) {
    lcd.clear();
    lcd.print("Blad WiFi!");
    lcd.setCursor(0, 1);
    lcd.print("Sprawdz haslo");
    while (true) delay(1000);
  }

  timeClient.begin();
  timeClient.update();

  lcd.clear();
  lcd.print("WiFi OK");
  beep(1);
  delay(1000);
}

// ===== LOOP =====

void loop() {
  timeClient.update();

  int min1 = -1, min2 = -1;
  String line = lines[currentLine];

  if (getDepartures(line, min1, min2)) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Linia " + line + ":");

    lcd.setCursor(0, 1);
    if (min1 == 0) lcd.print("teraz");
    else { lcd.print(min1); lcd.print(" min"); }

    if (min2 >= 0) {
      lcd.print("  ");
      lcd.print(min2);
      lcd.print(" min");
    }

    if (min1 <= ALARM_MIN) beep(3);
  } else {
    lcd.clear();
    lcd.print("Blad polaczenia");
    lcd.setCursor(0, 1);
    lcd.print("Retry za 30s...");
  }

  // Czekaj na odswiezenie, reaguj na przycisk
  unsigned long start = millis();
  while (millis() - start < REFRESH_MS) {
    if (buttonPressed()) {
      currentLine = (currentLine + 1) % LINE_COUNT;
      lcd.clear();
      lcd.print("Linia: ");
      lcd.print(lines[currentLine]);
      delay(500);
      break; // odswież natychmiast
    }
    delay(50);
  }
}
