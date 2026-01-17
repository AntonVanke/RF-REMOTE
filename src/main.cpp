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
U8G2* u8g2;  // 缓存U8G2指针，避免重复调用getU8g2()

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
PageState lastPage = PAGE_MENU;  // 记录上一次的页面状态

// 关于页面相关
unsigned long aboutStartTime = 0;
unsigned long aboutFrameCount = 0;
float currentFPS = 0;
unsigned long lastFPSUpdate = 0;

// 电池状态缓存
uint8_t lastBatteryPercent = 0;
bool lastIsCharging = false;
bool lastIsUSBPowered = false;
unsigned long lastBatteryUpdate = 0;
const unsigned long BATTERY_UPDATE_INTERVAL = 1000;  // 电池状态每1秒更新一次

// 优化：快速整数转字符串 (避免sprintf开销)
inline void intToStr(unsigned long val, char* buf, int width = 0) {
    char temp[12];
    int i = 0;
    if (val == 0) {
        temp[i++] = '0';
    } else {
        while (val > 0) {
            temp[i++] = '0' + (val % 10);
            val /= 10;
        }
    }
    // 填充前导零
    while (i < width) {
        temp[i++] = '0';
    }
    // 反转
    int j = 0;
    while (i > 0) {
        buf[j++] = temp[--i];
    }
    buf[j] = '\0';
}

// 优化：快速浮点转字符串 (1位小数)
inline void floatToStr(float val, char* buf) {
    int intPart = (int)val;
    int decPart = (int)((val - intPart) * 10);
    if (decPart < 0) decPart = -decPart;

    char intStr[8];
    intToStr(intPart, intStr);

    int i = 0;
    while (intStr[i]) buf[i] = intStr[i], i++;
    buf[i++] = '.';
    buf[i++] = '0' + decPart;
    buf[i] = '\0';
}

void setup() {
    Serial.begin(115200);
    delay(1000);  // 等待串口初始化

    Serial.println("\n==============================");
    Serial.println("RF遥控器启动中...");
    Serial.println("==============================");

    // 初始化显示
    display.scanI2C();
    display.begin();

    // 缓存U8G2指针
    u8g2 = display.getU8g2();

    // 初始化电池监测
    battery.begin();

    // 初始化按键
    buttons.begin();

    // 创建模块实例
    statusBar = new StatusBar(u8g2);
    menu = new Menu(u8g2, menuItems, MENU_ITEMS_COUNT);

    // 初始化电池状态显示
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
                        } else if (selection == 2) {
                            currentPage = PAGE_ABOUT;
                            // 重置FPS计数器
                            aboutStartTime = millis();
                            aboutFrameCount = 0;
                            currentFPS = 0;
                            lastFPSUpdate = millis();
                            Serial.println("进入: 关于页面");
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
        case PAGE_ABOUT:
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

    // 关于页面：强制每帧刷新以计算FPS
    if (currentPage == PAGE_ABOUT) {
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
        u8g2->clearBuffer();

        // 根据当前页面绘制不同内容
        switch (currentPage) {
            case PAGE_MENU:
                statusBar->draw("RF遥控器", lastBatteryPercent, lastIsCharging, lastIsUSBPowered);
                menu->draw();
                break;

            case PAGE_SIGNAL_RX:
                statusBar->draw("信号接收", lastBatteryPercent, lastIsCharging, lastIsUSBPowered);
                u8g2->setFont(u8g2_font_wqy12_t_gb2312);
                u8g2->drawUTF8(10, 30, "接收中...");
                u8g2->setFont(u8g2_font_6x10_tf);
                u8g2->drawStr(10, 50, "Long press UP to return");
                break;

            case PAGE_SIGNAL_TX:
                statusBar->draw("发送模式", lastBatteryPercent, lastIsCharging, lastIsUSBPowered);
                u8g2->setFont(u8g2_font_wqy12_t_gb2312);
                u8g2->drawUTF8(10, 30, "发送模式");
                u8g2->setFont(u8g2_font_6x10_tf);
                u8g2->drawStr(10, 50, "Long press UP to return");
                break;

            case PAGE_ABOUT:
                {
                    // 计算FPS
                    aboutFrameCount++;
                    unsigned long currentTime = millis();

                    // 每500ms更新一次FPS
                    if (currentTime - lastFPSUpdate >= 500) {
                        unsigned long elapsed = currentTime - aboutStartTime;
                        if (elapsed > 0) {
                            currentFPS = (aboutFrameCount * 1000.0f) / elapsed;
                        }
                        lastFPSUpdate = currentTime;
                    }

                    // 绘制关于页面
                    statusBar->draw("关于", lastBatteryPercent, lastIsCharging, lastIsUSBPowered);

                    // 使用小字体显示系统信息
                    u8g2->setFont(u8g2_font_6x10_tf);

                    // 第1行: FPS
                    char line1[24] = "FPS: ";
                    floatToStr(currentFPS, line1 + 5);
                    u8g2->drawStr(0, 28, line1);

                    // 第2行: CPU频率
                    char line2[24] = "CPU: ";
                    intToStr(getCpuFrequencyMhz(), line2 + 5);
                    int len2 = 5;
                    while (line2[len2]) len2++;
                    line2[len2++] = 'M';
                    line2[len2++] = 'H';
                    line2[len2++] = 'z';
                    line2[len2] = '\0';
                    u8g2->drawStr(0, 38, line2);

                    // 第3行: 剩余RAM
                    char line3[24] = "RAM: ";
                    intToStr(ESP.getFreeHeap() / 1024, line3 + 5);
                    int len3 = 5;
                    while (line3[len3]) len3++;
                    line3[len3++] = 'K';
                    line3[len3++] = 'B';
                    line3[len3] = '\0';
                    u8g2->drawStr(0, 48, line3);

                    // 第4行: Flash大小
                    char line4[24] = "Flash: ";
                    intToStr(ESP.getFlashChipSize() / 1024 / 1024, line4 + 7);
                    int len4 = 7;
                    while (line4[len4]) len4++;
                    line4[len4++] = 'M';
                    line4[len4++] = 'B';
                    line4[len4] = '\0';
                    u8g2->drawStr(0, 58, line4);

                    // 右侧信息
                    // 芯片型号
                    u8g2->drawStr(64, 28, "ESP32-C3");

                    // SDK版本
                    char sdkLine[16] = "SDK:";
                    const char* sdk = ESP.getSdkVersion();
                    int si = 4;
                    for (int j = 0; sdk[j] && si < 14; j++) {
                        sdkLine[si++] = sdk[j];
                    }
                    sdkLine[si] = '\0';
                    u8g2->drawStr(64, 38, sdkLine);

                    // 运行时间
                    char uptimeLine[16] = "Up:";
                    unsigned long uptime = millis() / 1000;
                    intToStr(uptime, uptimeLine + 3);
                    int ul = 3;
                    while (uptimeLine[ul]) ul++;
                    uptimeLine[ul++] = 's';
                    uptimeLine[ul] = '\0';
                    u8g2->drawStr(64, 48, uptimeLine);

                    // 电池电压 (需要加上0.27V二极管压降)
                    char voltLine[16] = "Bat:";
                    float voltage = battery.readVoltage() + 0.27f;  // 补偿二极管压降
                    int vInt = (int)voltage;
                    int vDec = (int)((voltage - vInt) * 100);
                    voltLine[4] = '0' + vInt;
                    voltLine[5] = '.';
                    voltLine[6] = '0' + (vDec / 10);
                    voltLine[7] = '0' + (vDec % 10);
                    voltLine[8] = 'V';
                    voltLine[9] = '\0';
                    u8g2->drawStr(64, 58, voltLine);
                }
                break;
        }

        u8g2->sendBuffer();
    }

    // 移除delay，最大化刷新率
    // delay(1);
}
