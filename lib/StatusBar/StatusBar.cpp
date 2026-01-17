/**
 * @file StatusBar.cpp
 * @brief OLED顶部状态栏模块实现
 */

#include "StatusBar.h"

StatusBar::StatusBar(U8G2* display) : _display(display) {}

void StatusBar::draw(const char* title, uint8_t batteryPercent, bool isCharging, bool isUSBPowered) {
    // 设置字体
    _display->setFont(u8g2_font_wqy12_t_gb2312);

    // 绘制标题
    _display->drawUTF8(0, 12, title);

    // 绘制电池图标 (右上角)
    drawBatteryIcon(128 - 40, 2, batteryPercent, isCharging, isUSBPowered);

    // 绘制分隔线
    _display->drawHLine(0, HEIGHT, 128);
}

void StatusBar::drawBatteryIcon(int x, int y, uint8_t percent, bool charging, bool usbPowered) {
    // USB供电模式
    if (usbPowered) {
        // 绘制USB图标 (简单的方框表示)
        _display->drawFrame(x, y, 14, 7);
        _display->drawBox(x + 14, y + 2, 2, 3);  // 电池凸起
        _display->drawBox(x + 1, y + 1, 12, 5);  // 满电池

        // 充电图标 (闪电)
        if (charging) {
            drawChargingIcon(x + 18, y);
        }

        // 显示 "USB" 字样
        _display->setFont(u8g2_font_5x7_tf);
        _display->drawStr(x + (charging ? 24 : 18), y + 6, "USB");
        return;
    }

    // 电池供电模式
    // 电池外框 (宽14, 高7)
    _display->drawFrame(x, y, 14, 7);
    _display->drawBox(x + 14, y + 2, 2, 3);  // 电池凸起

    // 电量填充 (根据百分比)
    int fillWidth = (percent * 12) / 100;
    if (fillWidth > 0) {
        _display->drawBox(x + 1, y + 1, fillWidth, 5);
    }

    // 充电图标 (闪电)
    if (charging) {
        drawChargingIcon(x + 18, y);
    }

    // 显示百分比
    _display->setFont(u8g2_font_5x7_tf);
    char percentStr[5];
    sprintf(percentStr, "%d%%", percent);
    _display->drawStr(x + (charging ? 24 : 18), y + 6, percentStr);
}

void StatusBar::drawChargingIcon(int x, int y) {
    // 简单的闪电符号
    _display->drawStr(x, y + 6, "~");
}
