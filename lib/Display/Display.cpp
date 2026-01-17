/**
 * @file Display.cpp
 * @brief OLED显示管理模块实现
 */

#include "Display.h"

Display::Display()
    : _u8g2(U8G2_R0, OLED_SCL_PIN, OLED_SDA_PIN, U8X8_PIN_NONE) {}

bool Display::begin() {
    Serial.println("初始化OLED显示屏...");

    if (!_u8g2.begin()) {
        Serial.println("错误: OLED初始化失败!");
        return false;
    }

    _u8g2.enableUTF8Print();
    Serial.println("OLED初始化成功");
    return true;
}

void Display::scanI2C() {
    Serial.println("\n开始扫描I2C设备...");
    Serial.print("SDA引脚: ");
    Serial.print(OLED_SDA_PIN);
    Serial.print(", SCL引脚: ");
    Serial.println(OLED_SCL_PIN);

    byte count = 0;
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

    for (byte i = 1; i < 127; i++) {
        Wire.beginTransmission(i);
        if (Wire.endTransmission() == 0) {
            Serial.print("发现I2C设备，地址: 0x");
            Serial.println(i, HEX);
            count++;
        }
    }

    Serial.print("扫描完成，找到 ");
    Serial.print(count);
    Serial.println(" 个设备\n");
}

U8G2* Display::getU8g2() {
    return &_u8g2;
}
