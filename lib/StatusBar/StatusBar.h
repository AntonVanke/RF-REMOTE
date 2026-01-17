/**
 * @file StatusBar.h
 * @brief OLED顶部状态栏模块
 *
 * 管理顶部状态栏的显示，包括标题、电池图标、电量百分比和页码
 */

#ifndef STATUS_BAR_H
#define STATUS_BAR_H

#include <U8g2lib.h>

class StatusBar {
public:
    /**
     * @brief 构造函数
     * @param display U8G2实例指针
     */
    StatusBar(U8G2* display);

    /**
     * @brief 绘制状态栏
     * @param title 标题文字
     * @param batteryPercent 电池电量百分比 (0-100)
     * @param isCharging 是否正在充电
     * @param isUSBPowered 是否USB供电
     */
    void draw(const char* title, uint8_t batteryPercent, bool isCharging, bool isUSBPowered);

private:
    U8G2* _display;

    /**
     * @brief 绘制电池图标
     */
    void drawBatteryIcon(int x, int y, uint8_t percent, bool charging, bool usbPowered);

    /**
     * @brief 绘制充电图标
     */
    void drawChargingIcon(int x, int y);

    static const int HEIGHT = 16;  // 状态栏高度
};

#endif // STATUS_BAR_H
