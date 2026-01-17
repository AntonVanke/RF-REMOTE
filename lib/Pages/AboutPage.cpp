#include "AboutPage.h"
#include <Arduino.h>
#include <esp32-hal-log.h>

static const char* TAG = "AboutPage";

AboutPage::AboutPage(U8G2* u8g2, BatteryMonitor* battery)
    : _u8g2(u8g2)
    , _battery(battery)
    , _startTime(0)
    , _frameCount(0)
    , _currentFPS(0)
    , _lastFPSUpdate(0)
{
}

void AboutPage::enter() {
    // 重置FPS计数器
    _startTime = millis();
    _frameCount = 0;
    _currentFPS = 0;
    _lastFPSUpdate = millis();
    ESP_LOGI(TAG, "进入: 关于页面");
}

bool AboutPage::update() {
    // 计算FPS
    _frameCount++;
    unsigned long currentTime = millis();

    // 每500ms更新一次FPS
    if (currentTime - _lastFPSUpdate >= 500) {
        unsigned long elapsed = currentTime - _startTime;
        if (elapsed > 0) {
            _currentFPS = (_frameCount * 1000.0f) / elapsed;
        }
        _lastFPSUpdate = currentTime;
    }

    return true;  // 关于页面总是需要重绘
}

void AboutPage::draw() {
    // 使用小字体显示系统信息
    _u8g2->setFont(u8g2_font_6x10_tf);

    // 第1行: FPS
    char line1[24] = "FPS: ";
    floatToStr(_currentFPS, line1 + 5);
    _u8g2->drawStr(0, 28, line1);

    // 第2行: CPU频率
    char line2[24] = "CPU: ";
    intToStr(getCpuFrequencyMhz(), line2 + 5);
    int len2 = 5;
    while (line2[len2]) len2++;
    line2[len2++] = 'M';
    line2[len2++] = 'H';
    line2[len2++] = 'z';
    line2[len2] = '\0';
    _u8g2->drawStr(0, 38, line2);

    // 第3行: 剩余RAM
    char line3[24] = "RAM: ";
    intToStr(ESP.getFreeHeap() / 1024, line3 + 5);
    int len3 = 5;
    while (line3[len3]) len3++;
    line3[len3++] = 'K';
    line3[len3++] = 'B';
    line3[len3] = '\0';
    _u8g2->drawStr(0, 48, line3);

    // 第4行: Flash大小
    char line4[24] = "Flash: ";
    intToStr(ESP.getFlashChipSize() / 1024 / 1024, line4 + 7);
    int len4 = 7;
    while (line4[len4]) len4++;
    line4[len4++] = 'M';
    line4[len4++] = 'B';
    line4[len4] = '\0';
    _u8g2->drawStr(0, 58, line4);

    // 右侧信息
    // 芯片型号
    _u8g2->drawStr(64, 28, "ESP32-C3");

    // SDK版本
    char sdkLine[16] = "SDK:";
    const char* sdk = ESP.getSdkVersion();
    int si = 4;
    for (int j = 0; sdk[j] && si < 14; j++) {
        sdkLine[si++] = sdk[j];
    }
    sdkLine[si] = '\0';
    _u8g2->drawStr(64, 38, sdkLine);

    // 运行时间
    char uptimeLine[16] = "Up:";
    unsigned long uptime = millis() / 1000;
    intToStr(uptime, uptimeLine + 3);
    int ul = 3;
    while (uptimeLine[ul]) ul++;
    uptimeLine[ul++] = 's';
    uptimeLine[ul] = '\0';
    _u8g2->drawStr(64, 48, uptimeLine);

    // 电池电压 (需要加上0.27V二极管压降)
    char voltLine[16] = "Bat:";
    float voltage = _battery->readVoltage() + 0.27f;  // 补偿二极管压降
    int vInt = (int)voltage;
    int vDec = (int)((voltage - vInt) * 100);
    voltLine[4] = '0' + vInt;
    voltLine[5] = '.';
    voltLine[6] = '0' + (vDec / 10);
    voltLine[7] = '0' + (vDec % 10);
    voltLine[8] = 'V';
    voltLine[9] = '\0';
    _u8g2->drawStr(64, 58, voltLine);
}

bool AboutPage::handleButton(ButtonEvent event) {
    switch (event) {
        case BTN_UP_LONG:
            ESP_LOGD(TAG, "按键: 上键长按 - 返回主菜单");
            return false;

        case BTN_UP_SHORT:
            ESP_LOGD(TAG, "按键: 上");
            return true;

        case BTN_DOWN_SHORT:
            ESP_LOGD(TAG, "按键: 下");
            return true;

        case BTN_OK_SHORT:
            ESP_LOGD(TAG, "按键: 确认");
            return true;

        default:
            return true;
    }
}

// 快速整数转字符串
void AboutPage::intToStr(unsigned long val, char* buf, int width) {
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

// 快速浮点转字符串 (1位小数)
void AboutPage::floatToStr(float val, char* buf) {
    int intPart = (int)val;
    int decPart = (int)((val - intPart) * 10);
    if (decPart < 0) decPart = -decPart;

    char intStr[8];
    intToStr(intPart, intStr);

    int i = 0;
    while (intStr[i]) {
        buf[i] = intStr[i];
        i++;
    }
    buf[i++] = '.';
    buf[i++] = '0' + decPart;
    buf[i] = '\0';
}
