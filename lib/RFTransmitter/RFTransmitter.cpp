/**
 * @file RFTransmitter.cpp
 * @brief RF信号发送模块实现
 */

#include "RFTransmitter.h"
#include <esp32-hal-log.h>

static const char* TAG = "RFTransmitter";

RFTransmitter::RFTransmitter()
    : _sending(false)
    , _repeatCount(10)
{
}

void RFTransmitter::begin() {
    ESP_LOGI(TAG, "初始化RF发送模块...");
    ESP_LOGI(TAG, "433MHz TX引脚: %d", RF_433_TX_PIN);
    ESP_LOGI(TAG, "315MHz TX引脚: %d", RF_315_TX_PIN);

    // 启用发送
    _rcSwitch433.enableTransmit(RF_433_TX_PIN);
    _rcSwitch315.enableTransmit(RF_315_TX_PIN);

    // 设置重复次数
    _rcSwitch433.setRepeatTransmit(_repeatCount);
    _rcSwitch315.setRepeatTransmit(_repeatCount);

    ESP_LOGI(TAG, "RF发送模块初始化完成");
}

void RFTransmitter::send(unsigned long code, unsigned int bits, unsigned int freq, unsigned int protocol, unsigned int pulseLength) {
    _sending = true;

    ESP_LOGI(TAG, "发送信号: %dMHz 编码:%lu 协议:%d 位数:%d 脉宽:%dus",
             freq, code, protocol, bits, pulseLength);

    if (freq == 433) {
        if (pulseLength > 0) {
            _rcSwitch433.setProtocol(protocol, pulseLength);
        } else {
            _rcSwitch433.setProtocol(protocol);
        }
        _rcSwitch433.send(code, bits);
    } else if (freq == 315) {
        if (pulseLength > 0) {
            _rcSwitch315.setProtocol(protocol, pulseLength);
        } else {
            _rcSwitch315.setProtocol(protocol);
        }
        _rcSwitch315.send(code, bits);
    } else {
        ESP_LOGW(TAG, "未知频率: %d", freq);
    }

    _sending = false;
    ESP_LOGI(TAG, "发送完成");
}

void RFTransmitter::setRepeatTransmit(int repeat) {
    _repeatCount = repeat;
    _rcSwitch433.setRepeatTransmit(repeat);
    _rcSwitch315.setRepeatTransmit(repeat);
    ESP_LOGD(TAG, "重复发送次数设置为: %d", repeat);
}
