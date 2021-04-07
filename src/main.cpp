#include <Adafruit_I2CDevice.h>
#include <Arduino.h>
#include <GxEPD2_3C.h>
#include <HTTPClient.h>

#include "../config.h"
#include "iCal.h"
#include "log.h"
#include "render.h"
#include "util.h"

GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT> display(GxEPD2_420c(PIN_CS, PIN_DC, PIN_RST, PIN_BUSY));
uint32_t millivolt = 0;

ICalEntry calenderEntries[CALENDER_SIZE];
size_t calenderEntryCount = 0;

void enableWiFi(const char* ssid, const char* password);
void disableWiFi();
time_t getTimestampBlocking();
void updateCalender(const char* calenderUrl);
void improveVoltage();
void hibernate(uint32_t seconds);
void error(uint32_t seconds, const char* title, const char* format, ...);

void setup()
{
    Serial.begin(115200);
    // Serial.setDebugOutput(true);
    // esp_log_level_set("*", ESP_LOG_INFO);

#ifdef PIN_VOLTAGE
    analogReadResolution(10);
    analogSetAttenuation(VOLTAGE_ATTEN);
    millivolt = analogReadMilliVolts(PIN_VOLTAGE) * VOLTAGE_MOD;
    if (millivolt < 2800) {
        hibernate(0); // sleep forever
    }
    createAsyncOneTimeTask("Voltage", improveVoltage);
    LOGI("Voltage", "%u.%02u V", millivolt / 1000, millivolt % 1000);
#endif

    display.init(115200, false, 2, false);
    display.setRotation(3);

#ifdef PIN_VOLTAGE
    if (millivolt < 3000) {
        error(3600 * 24, "Voltage", "%u.%02u V", millivolt / 1000, millivolt % 1000);
    }
#endif

#ifdef PIN_LED
    pinMode(PIN_LED, OUTPUT);
#endif
    pinMode(PIN_CS, OUTPUT);
    pinMode(PIN_DC, OUTPUT);
    pinMode(PIN_RST, OUTPUT);
    pinMode(PIN_BUSY, INPUT);
}

void loop()
{
#ifdef PIN_LED
    digitalWrite(PIN_LED, HIGH);
#endif
    enableWiFi(WIFI_SSID, WIFI_PASSWORD);
    configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER);
    updateCalender(CALENDER_URL);
    createAsyncOneTimeTask("disableWifi", disableWiFi);
#ifdef PIN_LED
    digitalWrite(PIN_LED, LOW);
#endif

    LOGI("main", "render calender with %u entries", calenderEntryCount);
    time_t timestamp = getTimestampBlocking();
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(0, 0);
    renderCalender(display, timestamp, calenderEntries, calenderEntryCount);

    tm currentTime;
    localtime_r(&timestamp, &currentTime);
    unsigned sleepTime = 7200 // 2 hours after midnight
        + (23 - currentTime.tm_hour) * 3600
        + (59 - currentTime.tm_min) * 60
        + (59 - currentTime.tm_sec);

    renderFooter(display, timestamp, sleepTime, millivolt);
    LOGI("main", "calender rendered, update screen");
    display.display();
    hibernate(sleepTime);
};

void enableWiFi(const char* ssid, const char* password)
{
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(WIFI_HOSTNAME);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
        error(3600, "WiFi", "SSID %s not reached", ssid);
    }
}

void disableWiFi()
{
    if (WiFi.getMode() != WIFI_OFF) {
        LOGI("WiFi", "disable WiFi");
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        LOGI("WiFi", "WiFi disabled");
    }
}

time_t lastTimestamp = 0;
time_t getTimestampBlocking()
{
    if (lastTimestamp > 1000) {
        return lastTimestamp;
    }

    for (int i = 0; i < 200; ++i) {
        time(&lastTimestamp);
        if (lastTimestamp > 1000) {
            return lastTimestamp;
        }
        delay(1);
    }

    error(3600, "NTP", "update though %s failed", NTP_SERVER);
    return 0;
}

void updateCalender(const char* calenderUrl)
{
    LOGI("HTTP", "HTTP start %s", calenderUrl);

    HTTPClient http;
    http.begin(calenderUrl);
    int httpStatus = http.GET();
    if (httpStatus != HTTP_CODE_OK) {
        error(3600, "HTTP", "HTTP error %d %s", httpStatus, calenderUrl);
    } else {
        LOGI("HTTP", "HTTP ok %d %s", httpStatus, calenderUrl);
    }

    calenderEntryCount = 0;
    auto earliestEntry = getTimestampBlocking() - 86400;
    auto result = readICalStream(http.getStreamPtr(), calenderEntries, calenderEntryCount, CALENDER_SIZE, earliestEntry);
    http.end();

    switch (result) {
    case ICAL_END:
        LOGI("HTTP", "calender read successfully");
        break;
    case ICAL_END_UNEXPECRED:
    default:
        error(3600, "HTTP", "connection ended unexpected: %s", calenderUrl);
        break;
    }
}

void improveVoltage()
{
    uint32_t sum = millivolt;
    uint16_t samples = millivolt > 0 ? 1 : 0;
    do {
        sum += analogReadMilliVolts(PIN_VOLTAGE) * VOLTAGE_MOD;
        millivolt = sum / ++samples;
        delay(8);
    } while (samples < 128);
    LOGI("Voltage", "voltage measurement completed at %u", millivolt);
}

void hibernate(uint32_t seconds)
{
    if (seconds > 0) {
        LOGI("main", "sleep %u seconds now!", seconds);
        esp_deep_sleep(seconds * 1000000LL);
    } else {
        LOGI("main", "sleep forever now!");
        esp_deep_sleep_start(); // forever
    }
}

void error(uint32_t seconds, const char* title, const char* format, ...)
{
    createAsyncOneTimeTask("disableWiFi", disableWiFi);

    char titleBuffer[15];
    snprintf(titleBuffer, sizeof(titleBuffer) - 1, "%s Error", title);

    va_list args;
    va_start(args, format);
    char messageBuffer[255];
    vsnprintf(messageBuffer, sizeof(messageBuffer) - 1, format, args);
    va_end(args);

    LOGE(title, messageBuffer);

    display.fillScreen(GxEPD_WHITE);
    renderError(display, titleBuffer, messageBuffer);
    renderFooter(display, lastTimestamp, seconds, millivolt);
    display.display();

    hibernate(seconds);
}