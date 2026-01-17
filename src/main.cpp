#include <Arduino.h>
#include "Display.h"
#include "StatusBar.h"
#include "Menu.h"
#include "BatteryMonitor.h"
#include "ButtonManager.h"
#include "pin_config.h"

// 全局对象
Display display;
BatteryMonitor battery;
ButtonManager buttons;
StatusBar* statusBar;
Menu* menu;

// 菜单配置
const char* menuItems[] = {
    "信号接收",
    "发送模式"
};
const int MENU_ITEMS_COUNT = 2;

// 当前页面状态
enum PageState {
    PAGE_MENU,          // 主菜单
    PAGE_SIGNAL_RX,     // 信号接收
    PAGE_SIGNAL_TX      // 发送模式
};
PageState currentPage = PAGE_MENU;
PageState lastPage = PAGE_MENU;  // 记录上一次的页面状态

// 电池状态缓存
uint8_t lastBatteryPercent = 0;
bool lastIsCharging = false;
bool lastIsUSBPowered = false;
unsigned long lastBatteryUpdate = 0;
const unsigned long BATTERY_UPDATE_INTERVAL = 1000;  // 电池状态每1秒更新一次

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

    // 初始化按键
    buttons.begin();

    // 创建模块实例
    statusBar = new StatusBar(display.getU8g2());
    menu = new Menu(display.getU8g2(), menuItems, MENU_ITEMS_COUNT);

    // 初始化电池状态显示
    lastBatteryPercent = battery.getBatteryPercent();
    lastIsCharging = battery.isCharging();
    lastIsUSBPowered = battery.isUSBPowered();

    // 绘制初始界面
    display.getU8g2()->clearBuffer();
    statusBar->draw("RF遥控器", lastBatteryPercent, lastIsCharging, lastIsUSBPowered);
    menu->draw();
    display.getU8g2()->sendBuffer();

    Serial.println("RF遥控器启动完成");
    Serial.println("==============================\n");
}

// 处理按键事件
void handleButtonEvent(ButtonEvent event) {
    switch (currentPage) {
        case PAGE_MENU:
            switch (event) {
                case BTN_UP_SHORT:
                    menu->previous();
                    Serial.println("按键: 上");
                    break;

                case BTN_DOWN_SHORT:
                    menu->next();
                    Serial.println("按键: 下");
                    break;

                case BTN_OK_SHORT:
                    {
                        int selection = menu->getCurrentSelection();
                        Serial.print("按键: 确认 - 选择了: ");
                        Serial.println(menuItems[selection]);

                        // 根据选择进入不同页面
                        if (selection == 0) {
                            currentPage = PAGE_SIGNAL_RX;
                            Serial.println("进入: 信号接收页面");
                        } else if (selection == 1) {
                            currentPage = PAGE_SIGNAL_TX;
                            Serial.println("进入: 发送模式页面");
                        }
                    }
                    break;

                case BTN_UP_LONG:
                    Serial.println("按键: 上键长按 (在主菜单，无操作)");
                    break;

                default:
                    break;
            }
            break;

        case PAGE_SIGNAL_RX:
        case PAGE_SIGNAL_TX:
            // 子页面处理
            switch (event) {
                case BTN_UP_LONG:
                    // 长按上键返回主菜单
                    currentPage = PAGE_MENU;
                    Serial.println("按键: 上键长按 - 返回主菜单");
                    break;

                case BTN_UP_SHORT:
                    Serial.println("按键: 上 (子页面)");
                    break;

                case BTN_DOWN_SHORT:
                    Serial.println("按键: 下 (子页面)");
                    break;

                case BTN_OK_SHORT:
                    Serial.println("按键: 确认 (子页面)");
                    break;

                default:
                    break;
            }
            break;
    }
}

void loop() {
    bool needRedraw = false;  // 是否需要重绘屏幕

    // 处理按键事件
    if (buttons.hasEvent()) {
        while (buttons.hasEvent()) {
            ButtonEvent event = buttons.getEvent();
            handleButtonEvent(event);
        }
        needRedraw = true;  // 按键事件触发重绘
    }

    // 检查页面是否改变
    if (currentPage != lastPage) {
        lastPage = currentPage;
        needRedraw = true;
    }

    // 定时更新电池状态 (每1秒)
    unsigned long now = millis();
    if (now - lastBatteryUpdate >= BATTERY_UPDATE_INTERVAL) {
        uint8_t batteryPercent = battery.getBatteryPercent();
        bool isCharging = battery.isCharging();
        bool isUSBPowered = battery.isUSBPowered();

        // 只有状态改变时才重绘
        if (batteryPercent != lastBatteryPercent ||
            isCharging != lastIsCharging ||
            isUSBPowered != lastIsUSBPowered) {
            lastBatteryPercent = batteryPercent;
            lastIsCharging = isCharging;
            lastIsUSBPowered = isUSBPowered;
            needRedraw = true;
        }

        lastBatteryUpdate = now;
    }

    // 只在需要时重绘屏幕
    if (needRedraw) {
        display.getU8g2()->clearBuffer();

        // 根据当前页面绘制不同内容
        switch (currentPage) {
            case PAGE_MENU:
                statusBar->draw("RF遥控器", lastBatteryPercent, lastIsCharging, lastIsUSBPowered);
                menu->draw();
                break;

            case PAGE_SIGNAL_RX:
                statusBar->draw("信号接收", lastBatteryPercent, lastIsCharging, lastIsUSBPowered);
                // TODO: 绘制信号接收页面内容
                display.getU8g2()->setFont(u8g2_font_wqy12_t_gb2312);
                display.getU8g2()->drawUTF8(10, 30, "接收中...");
                display.getU8g2()->setFont(u8g2_font_6x10_tf);
                display.getU8g2()->drawStr(10, 50, "Long press UP to return");
                break;

            case PAGE_SIGNAL_TX:
                statusBar->draw("发送模式", lastBatteryPercent, lastIsCharging, lastIsUSBPowered);
                // TODO: 绘制发送模式页面内容
                display.getU8g2()->setFont(u8g2_font_wqy12_t_gb2312);
                display.getU8g2()->drawUTF8(10, 30, "发送模式");
                display.getU8g2()->setFont(u8g2_font_6x10_tf);
                display.getU8g2()->drawStr(10, 50, "Long press UP to return");
                break;
        }

        display.getU8g2()->sendBuffer();
    }

    // 极短延迟，保持高响应性
    delay(1);
}
