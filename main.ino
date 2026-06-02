#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
const char* serverUrl = "https://your-app.onrender.com/api/touch";

#define TOUCH_PIN   T0   // GPIO 4
#define BUZZER_PIN  25
#define TOUCH_THRESHOLD 40

// NTP
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset = 19800;  // IST = UTC+5:30
const int   dstOffset = 0;

bool lastTouched = false;

void beep(int times, int freq, int dur) {
  for (int i = 0; i < times; i++) {
    tone(BUZZER_PIN, freq, dur);
    delay(dur + 80);
  }
}

void postTouch() {
  if (WiFi.status() != WL_CONNECTED) return;

  struct tm timeinfo;
  char dateStr[20], timeStr[20];
  if (getLocalTime(&timeinfo)) {
    strftime(dateStr, sizeof(dateStr), "%Y-%m-%d", &timeinfo);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
  } else {
    strcpy(dateStr, "unknown");
    strcpy(timeStr, "unknown");
  }

  HTTPClient http;
  http.begin(serverUrl);
  http.addHeader("Content-Type", "application/json");

  String body = "{\"date\":\"" + String(dateStr) + "\",\"time\":\"" + String(timeStr) + "\"}";
  int code = http.POST(body);

  Serial.printf("POST %s -> %d\n", serverUrl, code);
  http.end();
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);

  Serial.print("Connecting WiFi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

  configTime(gmtOffset, dstOffset, ntpServer);
  Serial.println("NTP synced");

  beep(2, 1000, 100);
}

void loop() {
  int val = touchRead(TOUCH_PIN);
  bool touched = val < TOUCH_THRESHOLD;

  if (touched && !lastTouched) {
    Serial.printf("Touch detected! (val=%d)\n", val);
    beep(1, 1500, 200);
    postTouch();
  }

  lastTouched = touched;
  delay(50);
}
