#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

const char* ssid      = "sam";
const char* password  = "SANS2186";
const char* serverUrl = "https://pizoboy.onrender.com/api/touch";

#define TOUCH_PIN   4    // GPIO 4 -- digital touch sensor (HIGH=touched, LOW=not)
#define BUZZER_PIN  25

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset = 19800;   // IST UTC+5:30
const int   dstOffset = 0;

bool lastTouched = false;
int  debounce    = 0;

void beep() {
  tone(BUZZER_PIN, 1500, 250);
  delay(300);
}

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Reconnecting");
  WiFi.begin(ssid, password);
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500); Serial.print(".");
  }
  Serial.println();
}

void postTouch() {
  connectWiFi();
  if (WiFi.status() != WL_CONNECTED) return;

  struct tm t;
  char dateStr[20], timeStr[20];
  if (getLocalTime(&t)) {
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &t);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &t);
  } else {
    strcpy(dateStr, "unknown");
    strcpy(timeStr, "unknown");
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.setTimeout(8000);
  http.addHeader("Content-Type", "application/json");
  String body = "{\"date\":\"" + String(dateStr) + "\",\"time\":\"" + String(timeStr) + "\"}";
  int code = http.POST(body);
  Serial.printf("POST -> %d\n", code);
  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(TOUCH_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  Serial.print("Connecting WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi: " + WiFi.localIP().toString());

  configTime(gmtOffset, dstOffset, ntpServer);
  Serial.println("NTP synced");

  beep();
}

void loop() {
  int  val      = digitalRead(TOUCH_PIN);   // 1 = touched, 0 = not touched
  bool rawTouch = (val == HIGH);

  debounce = rawTouch ? min(debounce + 1, 5) : max(debounce - 1, 0);
  bool touched = debounce >= 2;

  if (touched && !lastTouched) {
    lastTouched = true;
    Serial.printf("Touch! val=%d\n", val);
    beep();
    postTouch();
  } else if (!touched && lastTouched) {
    lastTouched = false;
    Serial.println("Released -- waiting");
  }

  delay(30);
}
