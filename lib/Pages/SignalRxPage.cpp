#include "SignalRxPage.h"
#include <Arduino.h>

SignalRxPage::SignalRxPage(U8G2* u8g2)
    : _u8g2(u8g2)
{
}

void SignalRxPage::enter() {
    Serial.println("进入: 信号接收页面");
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
    // if (_receivedCode != 0) {
    //     显示协议、编码、位数等
    // }
}

bool SignalRxPage::handleButton(ButtonEvent event) {
    switch (event) {
        case BTN_UP_LONG:
            // 长按上键返回主菜单
            Serial.println("按键: 上键长按 - 返回主菜单");
            // TODO: 停止RF接收
            return false;

        case BTN_UP_SHORT:
            Serial.println("按键: 上 (信号接收页面)");
            return true;

        case BTN_DOWN_SHORT:
            Serial.println("按键: 下 (信号接收页面)");
            return true;

        case BTN_OK_SHORT:
            Serial.println("按键: 确认 (信号接收页面)");
            // TODO: 保存接收到的信号
            return true;

        default:
            return true;
    }
}
