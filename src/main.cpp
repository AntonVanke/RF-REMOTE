#include <Arduino.h>
#include <esp32-hal-log.h>
#include "Display.h"
#include "StatusBar.h"
#include "Menu.h"
#include "BatteryMonitor.h"
#include "ButtonManager.h"
#include "pin_config.h"

// RF模块
#include "RFReceiver.h"
#include "RFTransmitter.h"
#include "SignalStorage.h"

// 页面模块
#include "Page.h"
#include "AboutPage.h"
#include "SignalRxPage.h"
#include "SignalTxPage.h"

// 日志标签
static const char* TAG = "Main";

// 全局对象
Display display;
BatteryMonitor battery;
ButtonManager buttons;
RFReceiver rfReceiver;
RFTransmitter rfTransmitter;
SignalStorage signalStorage;
StatusBar* statusBar;
Menu* menu;
U8G2* u8g2;

// 页面对象
AboutPage* aboutPage;
SignalRxPage* signalRxPage;
SignalTxPage* signalTxPage;
Page* currentPageObj = nullptr;

// 菜单配置
const char* menuItems[] = {
    "信号接收",
    "发送模式",
    "关于"
};
const int MENU_ITEMS_COUNT = 3;

// 页面状态
enum PageState {
    PAGE_MENU,
    PAGE_SIGNAL_RX,
    PAGE_SIGNAL_TX,
    PAGE_ABOUT
};
PageState currentPage = PAGE_MENU;
PageState lastPage = PAGE_MENU;

// ============ 局部刷新配置 ============
// 屏幕区域定义 (tile坐标，每个tile=8像素)
const uint8_t SCREEN_WIDTH_TILES = 16;   // 128/8 = 16 tiles
const uint8_t STATUSBAR_TILES = 2;       // 状态栏高度: 16像素 = 2 tiles
const uint8_t CONTENT_START_TILE = 2;    // 内容区起始tile
const uint8_t CONTENT_TILES = 6;         // 内容区高度: 48像素 = 6 tiles

// 脏区域标志
bool statusBarDirty = true;   // 状态栏需要刷新
bool contentDirty = true;     // 内容区需要刷新
bool fullRefresh = true;      // 需要全屏刷新

// 电池状态缓存
uint8_t lastBatteryPercent = 0;
bool lastIsCharging = false;
bool lastIsUSBPowered = false;
unsigned long lastBatteryUpdate = 0;
const unsigned long BATTERY_UPDATE_INTERVAL = 1000;

// 页面标题缓存 (用于检测标题变化)
const char* lastTitle = nullptr;

// ============ 局部刷新函数 ============

// 清除指定区域 (像素坐标)
inline void clearArea(int16_t x, int16_t y, int16_t w, int16_t h) {
    u8g2->setDrawColor(0);
    u8g2->drawBox(x, y, w, h);
    u8g2->setDrawColor(1);
}

// 只刷新状态栏区域
void refreshStatusBar(const char* title) {
    // 清除状态栏区域
    clearArea(0, 0, 128, 16);

    // 绘制状态栏
    statusBar->draw(title, lastBatteryPercent, lastIsCharging, lastIsUSBPowered);

    // 只发送状态栏区域
    u8g2->updateDisplayArea(0, 0, SCREEN_WIDTH_TILES, STATUSBAR_TILES);
}

// 只刷新内容区域
void refreshContent() {
    // 清除内容区域
    clearArea(0, 16, 128, 48);

    // 重绘分隔线 (因为Y=16在边界上，清除内容区时会被清掉)
    u8g2->drawHLine(0, 16, 128);

    // 绘制内容
    if (currentPage == PAGE_MENU) {
        menu->draw();
    } else if (currentPageObj) {
        currentPageObj->draw();
    }

    // 只发送内容区域
    u8g2->updateDisplayArea(0, CONTENT_START_TILE, SCREEN_WIDTH_TILES, CONTENT_TILES);
}

// 全屏刷新
void refreshAll(const char* title) {
    u8g2->clearBuffer();

    // 绘制状态栏
    statusBar->draw(title, lastBatteryPercent, lastIsCharging, lastIsUSBPowered);

    // 绘制内容
    if (currentPage == PAGE_MENU) {
        menu->draw();
    } else if (currentPageObj) {
        currentPageObj->draw();
    }

    u8g2->sendBuffer();
}

// ============ 辅助函数 ============

Page* getPageByIndex(int index) {
    switch (index) {
        case 0: return signalRxPage;
        case 1: return signalTxPage;
        case 2: return aboutPage;
        default: return nullptr;
    }
}

PageState getPageStateByIndex(int index) {
    switch (index) {
        case 0: return PAGE_SIGNAL_RX;
        case 1: return PAGE_SIGNAL_TX;
        case 2: return PAGE_ABOUT;
        default: return PAGE_MENU;
    }
}

// ============ 主程序 ============

void setup() {
    Serial.begin(115200);
    delay(1000);

    ESP_LOGI(TAG, "==============================");
    ESP_LOGI(TAG, "RF遥控器启动中...");
    ESP_LOGI(TAG, "==============================");

    display.scanI2C();
    display.begin();
    u8g2 = display.getU8g2();

    battery.begin();
    buttons.begin();

    // 初始化RF接收、发送和信号存储
    rfReceiver.begin();
    rfTransmitter.begin();
    signalStorage.begin();

    statusBar = new StatusBar(u8g2);
    menu = new Menu(u8g2, menuItems, MENU_ITEMS_COUNT);

    aboutPage = new AboutPage(u8g2, &battery);
    signalRxPage = new SignalRxPage(u8g2, &rfReceiver, &signalStorage);
    signalTxPage = new SignalTxPage(u8g2, &signalStorage, &rfTransmitter);

    lastBatteryPercent = battery.getBatteryPercent();
    lastIsCharging = battery.isCharging();
    lastIsUSBPowered = battery.isUSBPowered();

    // 初始全屏刷新
    refreshAll("RF遥控器");
    lastTitle = "RF遥控器";
    fullRefresh = false;
    statusBarDirty = false;
    contentDirty = false;

    ESP_LOGI(TAG, "RF遥控器启动完成 (局部刷新已启用)");
    ESP_LOGI(TAG, "==============================");
}

void handleButtonEvent(ButtonEvent event) {
    if (currentPage == PAGE_MENU) {
        switch (event) {
            case BTN_UP_SHORT:
                menu->previous();
                contentDirty = true;
                ESP_LOGD(TAG, "按键: 上");
                break;

            case BTN_DOWN_SHORT:
                menu->next();
                contentDirty = true;
                ESP_LOGD(TAG, "按键: 下");
                break;

            case BTN_OK_SHORT:
                {
                    int selection = menu->getCurrentSelection();
                    ESP_LOGD(TAG, "按键: 确认 - 选择了: %s", menuItems[selection]);

                    currentPage = getPageStateByIndex(selection);
                    currentPageObj = getPageByIndex(selection);
                    if (currentPageObj) {
                        currentPageObj->enter();
                    }
                    fullRefresh = true;
                }
                break;

            case BTN_UP_LONG:
                ESP_LOGD(TAG, "按键: 上键长按 (在主菜单，无操作)");
                break;

            default:
                break;
        }
    } else {
        if (currentPageObj) {
            bool handled = currentPageObj->handleButton(event);
            if (!handled) {
                currentPage = PAGE_MENU;
                currentPageObj = nullptr;
                fullRefresh = true;
            } else {
                contentDirty = true;
            }
        }
    }
}

void loop() {
    // 处理按键事件
    if (buttons.hasEvent()) {
        while (buttons.hasEvent()) {
            ButtonEvent event = buttons.getEvent();
            handleButtonEvent(event);
        }
    }

    // 检查页面是否改变
    if (currentPage != lastPage) {
        lastPage = currentPage;
        fullRefresh = true;
    }

    // 页面更新
    if (currentPageObj) {
        if (currentPageObj->update()) {
            contentDirty = true;  // 页面更新只刷新内容区
        }
        if (currentPageObj->needsConstantRefresh()) {
            contentDirty = true;
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
            statusBarDirty = true;  // 电池变化只刷新状态栏
        }

        lastBatteryUpdate = now;
    }

    // 获取当前标题
    const char* currentTitle = (currentPage == PAGE_MENU) ? "RF遥控器" :
                               (currentPageObj ? currentPageObj->getTitle() : "RF遥控器");

    // ============ 智能刷新 ============
    if (fullRefresh) {
        // 全屏刷新
        refreshAll(currentTitle);
        lastTitle = currentTitle;
        fullRefresh = false;
        statusBarDirty = false;
        contentDirty = false;
    } else {
        // 局部刷新
        if (statusBarDirty) {
            refreshStatusBar(currentTitle);
            lastTitle = currentTitle;
            statusBarDirty = false;
        }

        if (contentDirty) {
            refreshContent();
            contentDirty = false;
        }
    }
}
