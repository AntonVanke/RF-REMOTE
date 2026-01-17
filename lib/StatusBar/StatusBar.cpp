/**
 * @file StatusBar.cpp
 * @brief OLED顶部状态栏模块实现 (优化版)
 */

#include "StatusBar.h"

// 缓存上次的百分比字符串，避免频繁sprintf
static char _cachedPercentStr[5] = "0%";
static uint8_t _cachedPercent = 255;  // 初始化为无效值

StatusBar::StatusBar(U8G2* display) : _display(display) {}

void StatusBar::draw(const char* title, uint8_t batteryPercent, bool isCharging, bool isUSBPowered) {
    // 设置字体 (整个状态栏使用同一字体减少切换)
    _display->setFont(u8g2_font_wqy12_t_gb2312);

    // 绘制标题
    _display->drawUTF8(0, 12, title);

    // 绘制电池图标 (右上角)
    drawBatteryIcon(128 - 40, 2, batteryPercent, isCharging, isUSBPowered);

    // 绘制分隔线
    _display->drawHLine(0, HEIGHT, 128);
}

void StatusBar::drawBatteryIcon(int x, int y, uint8_t percent, bool charging, bool usbPowered) {
    // 电池外框 (两种模式通用)
    _display->drawFrame(x, y, 14, 7);
    _display->drawBox(x + 14, y + 2, 2, 3);  // 电池凸起

    // USB供电模式
    if (usbPowered) {
        _display->drawBox(x + 1, y + 1, 12, 5);  // 满电池

        // 充电图标 (闪电)
        if (charging) {
            drawChargingIcon(x + 18, y);
        }

        // 显示 "USB" 字样 (使用同一字体，稍小)
        _display->setFont(u8g2_font_5x7_tf);
        _display->drawStr(x + (charging ? 24 : 18), y + 6, "USB");
        return;
    }

    // 电池供电模式
    // 电量填充 (根据百分比)
    int fillWidth = (percent * 12) / 100;
    if (fillWidth > 0) {
        _display->drawBox(x + 1, y + 1, fillWidth, 5);
    }

    // 充电图标 (闪电)
    if (charging) {
        drawChargingIcon(x + 18, y);
    }

    // 显示百分比 (使用缓存减少sprintf调用)
    _display->setFont(u8g2_font_5x7_tf);

    // 只有百分比变化时才重新格式化
    if (percent != _cachedPercent) {
        _cachedPercent = percent;
        // 使用简单的整数转字符串，避免sprintf开销
        if (percent >= 100) {
            _cachedPercentStr[0] = '1';
            _cachedPercentStr[1] = '0';
            _cachedPercentStr[2] = '0';
            _cachedPercentStr[3] = '%';
            _cachedPercentStr[4] = '\0';
        } else if (percent >= 10) {
            _cachedPercentStr[0] = '0' + (percent / 10);
            _cachedPercentStr[1] = '0' + (percent % 10);
            _cachedPercentStr[2] = '%';
            _cachedPercentStr[3] = '\0';
        } else {
            _cachedPercentStr[0] = '0' + percent;
            _cachedPercentStr[1] = '%';
            _cachedPercentStr[2] = '\0';
        }
    }

    _display->drawStr(x + (charging ? 24 : 18), y + 6, _cachedPercentStr);
}

void StatusBar::drawChargingIcon(int x, int y) {
    // 简单的闪电符号
    _display->drawStr(x, y + 6, "~");
}
