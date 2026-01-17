#include "SignalTxPage.h"
#include <Arduino.h>

SignalTxPage::SignalTxPage(U8G2* u8g2)
    : _u8g2(u8g2)
{
}

void SignalTxPage::enter() {
    Serial.println("进入: 发送模式页面");
    // TODO: 加载已保存的信号列表
}

void SignalTxPage::draw() {
    // 中文提示
    _u8g2->setFont(u8g2_font_wqy12_t_gb2312);
    _u8g2->drawUTF8(10, 30, "发送模式");

    // 英文提示
    _u8g2->setFont(u8g2_font_6x10_tf);
    _u8g2->drawStr(10, 50, "Long press UP to return");

    // TODO: 显示已保存的信号列表
    // 用户可以选择信号并发送
}

bool SignalTxPage::handleButton(ButtonEvent event) {
    switch (event) {
        case BTN_UP_LONG:
            // 长按上键返回主菜单
            Serial.println("按键: 上键长按 - 返回主菜单");
            return false;

        case BTN_UP_SHORT:
            Serial.println("按键: 上 (发送模式页面)");
            // TODO: 选择上一个信号
            return true;

        case BTN_DOWN_SHORT:
            Serial.println("按键: 下 (发送模式页面)");
            // TODO: 选择下一个信号
            return true;

        case BTN_OK_SHORT:
            Serial.println("按键: 确认 (发送模式页面)");
            // TODO: 发送选中的信号
            return true;

        default:
            return true;
    }
}
