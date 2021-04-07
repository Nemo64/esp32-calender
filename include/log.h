#pragma once
#include <Arduino.h>

#define LOGI(tag, ...) Serial.printf("I (%u) %s: ", esp_log_timestamp(), tag); Serial.printf(__VA_ARGS__); Serial.println();
#define LOGE(tag, ...) Serial.printf("E (%u) %s: ", esp_log_timestamp(), tag); Serial.printf(__VA_ARGS__); Serial.println();