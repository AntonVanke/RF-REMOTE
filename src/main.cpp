#include <Arduino.h>
#include "Display.h"
#include "StatusBar.h"
#include "Menu.h"
#include "BatteryMonitor.h"
#include "pin_config.h"

// 全局对象
Display display;
BatteryMonitor battery;
StatusBar* statusBar;
Menu* menu;

// 菜单配置
const char* menuItems[] = {
    "信号接收",
    "发送模式"
};
const int MENU_ITEMS_COUNT = 2;

void setup() {
    Serial.begin(115200);
    delay(1000);  // 等待串口初始化

    Serial.println("\n==============================");
    Serial.println("RF遥控器启动中...");
    Serial.println("==============================");

    // 初始化显示
    display.scanI2C();
    display.begin();

    // 初始化电池监测
    battery.begin();

    // 创建模块实例
    statusBar = new StatusBar(display.getU8g2());
    menu = new Menu(display.getU8g2(), menuItems, MENU_ITEMS_COUNT);

    Serial.println("RF遥控器启动完成");
    Serial.println("==============================\n");
}

void loop() {
    // 获取电池状态
    uint8_t batteryPercent = battery.getBatteryPercent();
    bool isCharging = battery.isCharging();
    bool isUSBPowered = battery.isUSBPowered();

    // 绘制屏幕
    display.getU8g2()->clearBuffer();

    statusBar->draw("RF遥控器", batteryPercent, isCharging, isUSBPowered);
    menu->draw();

    display.getU8g2()->sendBuffer();

    delay(100);
}
