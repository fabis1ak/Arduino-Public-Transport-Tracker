# Arduino Tram Tracker

Wyświetla czas do następnych tramwajów na wyświetlaczu LCD. Dane pobiera z API ZTM Warszawa.

## Co robi

Łączy się z WiFi, pobiera rozkład jazdy i pokazuje za ile minut przyjedzie tramwaj. Przyciskiem można przełączać między liniami. Buzzer piszczy gdy tramwaj jest już blisko.

## Czego potrzebujesz

Arduino UNO R4 WiFi  
Wyświetlacz LCD 16x2 z I2C  
Buzzer pasywny  
Przycisk

## Biblioteki

NTPClient  
ArduinoJson  
LiquidCrystal I2C

## Podłączenie

Wyświetlacz LCD na I2C (SDA/SCL)  
Przycisk między pinem 2 a GND  
Buzzer między pinem 3 a GND

## Konfiguracja

W pliku `TramTimer.ino` uzupełnij:

```cpp
const char* ssid     = "nazwa_sieci";
const char* password = "haslo";
const char* apiKey   = "twoj_klucz_api";
String stopId        = "id_przystanku";
String stopNr        = "numer_przystanku";
String lines[]       = {"22", "17", "8"};
```

Klucz API i dane przystanku znajdziesz na api.um.warszawa.pl
