/**
 * @file Display.cpp
 * @brief OLED显示管理模块实现
 */

#include "Display.h"
#include <esp32-hal-log.h>

static const char* TAG = "Display";

Display::Display()
    #ifdef USE_HW_I2C
        : _u8g2(U8G2_R0, U8X8_PIN_NONE) {
        // 硬件I2C构造函数
    #else
        : _u8g2(U8G2_R0, OLED_SCL_PIN, OLED_SDA_PIN, U8X8_PIN_NONE) {
        // 软件I2C构造函数 (scl, sda, reset)
    #endif
}

bool Display::begin() {
    #ifdef USE_HW_I2C
        ESP_LOGI(TAG, "初始化OLED显示屏 (硬件I2C, 1MHz)...");

        // 设置I2C时钟频率为1MHz (ESP32-C3支持)
        Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
        Wire.setClock(1000000);  // 1MHz - SSD1306最高支持
    #else
        ESP_LOGI(TAG, "初始化OLED显示屏 (软件I2C)...");
    #endif

    if (!_u8g2.begin()) {
        ESP_LOGE(TAG, "OLED初始化失败!");
        return false;
    }

    _u8g2.enableUTF8Print();
    _u8g2.setContrast(255);

    // 设置I2C时钟 (对软件I2C也有效)
    _u8g2.setBusClock(400000);  // 400kHz

    ESP_LOGI(TAG, "OLED初始化成功");
    return true;
}

void Display::scanI2C() {
    ESP_LOGD(TAG, "开始扫描I2C设备...");
    ESP_LOGD(TAG, "SDA引脚: %d, SCL引脚: %d", OLED_SDA_PIN, OLED_SCL_PIN);

    byte count = 0;
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    for (byte i = 1; i < 127; i++) {
        Wire.beginTransmission(i);
        if (Wire.endTransmission() == 0) {
            ESP_LOGD(TAG, "发现I2C设备，地址: 0x%02X", i);
            count++;
        }
    }

    ESP_LOGD(TAG, "扫描完成，找到 %d 个设备", count);
}

U8G2* Display::getU8g2() {
    return &_u8g2;
}
