#include "SignalRxPage.h"
#include <Arduino.h>
#include <esp32-hal-log.h>

static const char* TAG = "SignalRx";

SignalRxPage::SignalRxPage(U8G2* u8g2)
    : _u8g2(u8g2)
{
}

void SignalRxPage::enter() {
    ESP_LOGI(TAG, "进入: 信号接收页面");
    // TODO: 启动RF接收
}

void SignalRxPage::draw() {
    // 中文提示
    _u8g2->setFont(u8g2_font_wqy12_t_gb2312);
    _u8g2->drawUTF8(10, 30, "接收中...");

    // 英文提示
    _u8g2->setFont(u8g2_font_6x10_tf);
    _u8g2->drawStr(10, 50, "Long press UP to return");

    // TODO: 显示接收到的信号信息
}

bool SignalRxPage::handleButton(ButtonEvent event) {
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
