#include <Arduino.h>
#include "Display.h"
#include "StatusBar.h"
#include "Menu.h"
#include "BatteryMonitor.h"
#include "ButtonManager.h"
#include "pin_config.h"

// 页面模块
#include "Page.h"
#include "AboutPage.h"
#include "SignalRxPage.h"
#include "SignalTxPage.h"

// 全局对象
Display display;
BatteryMonitor battery;
ButtonManager buttons;
StatusBar* statusBar;
Menu* menu;
U8G2* u8g2;  // 缓存U8G2指针

// 页面对象
AboutPage* aboutPage;
SignalRxPage* signalRxPage;
SignalTxPage* signalTxPage;
Page* currentPageObj = nullptr;  // 当前页面对象指针

// 菜单配置
const char* menuItems[] = {
    "信号接收",
    "发送模式",
    "关于"
};
const int MENU_ITEMS_COUNT = 3;

// 当前页面状态
enum PageState {
    PAGE_MENU,          // 主菜单
    PAGE_SIGNAL_RX,     // 信号接收
    PAGE_SIGNAL_TX,     // 发送模式
    PAGE_ABOUT          // 关于页面
};
PageState currentPage = PAGE_MENU;
PageState lastPage = PAGE_MENU;

// 电池状态缓存
uint8_t lastBatteryPercent = 0;
bool lastIsCharging = false;
bool lastIsUSBPowered = false;
unsigned long lastBatteryUpdate = 0;
const unsigned long BATTERY_UPDATE_INTERVAL = 1000;

// 根据菜单选择获取对应的页面对象
Page* getPageByIndex(int index) {
    switch (index) {
        case 0: return signalRxPage;
        case 1: return signalTxPage;
        case 2: return aboutPage;
        default: return nullptr;
    }
}

// 根据菜单选择获取对应的页面状态
PageState getPageStateByIndex(int index) {
    switch (index) {
        case 0: return PAGE_SIGNAL_RX;
        case 1: return PAGE_SIGNAL_TX;
        case 2: return PAGE_ABOUT;
        default: return PAGE_MENU;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n==============================");
    Serial.println("RF遥控器启动中...");
    Serial.println("==============================");

    // 初始化显示
    display.scanI2C();
    display.begin();
    u8g2 = display.getU8g2();

    // 初始化电池监测
    battery.begin();

    // 初始化按键
    buttons.begin();

    // 创建UI模块
    statusBar = new StatusBar(u8g2);
    menu = new Menu(u8g2, menuItems, MENU_ITEMS_COUNT);

    // 创建页面对象
    aboutPage = new AboutPage(u8g2, &battery);
    signalRxPage = new SignalRxPage(u8g2);
    signalTxPage = new SignalTxPage(u8g2);

    // 初始化电池状态
    lastBatteryPercent = battery.getBatteryPercent();
    lastIsCharging = battery.isCharging();
    lastIsUSBPowered = battery.isUSBPowered();

    // 绘制初始界面
    u8g2->clearBuffer();
    statusBar->draw("RF遥控器", lastBatteryPercent, lastIsCharging, lastIsUSBPowered);
    menu->draw();
    u8g2->sendBuffer();

    Serial.println("RF遥控器启动完成");
    Serial.println("==============================\n");
}

// 处理按键事件
void handleButtonEvent(ButtonEvent event) {
    if (currentPage == PAGE_MENU) {
        // 主菜单处理
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

                    // 进入子页面
                    currentPage = getPageStateByIndex(selection);
                    currentPageObj = getPageByIndex(selection);
                    if (currentPageObj) {
                        currentPageObj->enter();
                    }
                }
                break;

            case BTN_UP_LONG:
                Serial.println("按键: 上键长按 (在主菜单，无操作)");
                break;

            default:
                break;
        }
    } else {
        // 子页面处理 - 委托给页面对象
        if (currentPageObj) {
            bool handled = currentPageObj->handleButton(event);
            if (!handled) {
                // 页面请求返回主菜单
                currentPage = PAGE_MENU;
                currentPageObj = nullptr;
            }
        }
    }
}

void loop() {
    bool needRedraw = false;

    // 处理按键事件
    if (buttons.hasEvent()) {
        while (buttons.hasEvent()) {
            ButtonEvent event = buttons.getEvent();
            handleButtonEvent(event);
        }
        needRedraw = true;
    }

    // 检查页面是否改变
    if (currentPage != lastPage) {
        lastPage = currentPage;
        needRedraw = true;
    }

    // 页面更新
    if (currentPageObj) {
        if (currentPageObj->update()) {
            needRedraw = true;
        }
        if (currentPageObj->needsConstantRefresh()) {
            needRedraw = true;
        }
    }

    // 定时更新电池状态
    unsigned long now = millis();
    if (now - lastBatteryUpdate >= BATTERY_UPDATE_INTERVAL) {
        uint8_t batteryPercent = battery.getBatteryPercent();
        bool isCharging = battery.isCharging();
        bool isUSBPowered = battery.isUSBPowered();

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

    // 重绘屏幕
    if (needRedraw) {
        u8g2->clearBuffer();

        if (currentPage == PAGE_MENU) {
            // 主菜单
            statusBar->draw("RF遥控器", lastBatteryPercent, lastIsCharging, lastIsUSBPowered);
            menu->draw();
        } else if (currentPageObj) {
            // 子页面
            statusBar->draw(currentPageObj->getTitle(), lastBatteryPercent, lastIsCharging, lastIsUSBPowered);
            currentPageObj->draw();
        }

        u8g2->sendBuffer();
    }
}
