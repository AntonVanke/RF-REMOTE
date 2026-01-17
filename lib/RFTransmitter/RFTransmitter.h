/**
 * @file RFTransmitter.h
 * @brief RF信号发送模块，支持433/315MHz双频发送
 */

#ifndef RF_TRANSMITTER_H
#define RF_TRANSMITTER_H

#include <Arduino.h>
#include "RCSwitch433.h"
#include "RCSwitch315.h"
#include "pin_config.h"

class RFTransmitter {
public:
    RFTransmitter();

    /**
     * 初始化RF发送模块
     */
    void begin();

    /**
     * 发送RF信号
     * @param code 编码值
     * @param bits 位长度
     * @param freq 频率 (433/315)
     * @param protocol 协议类型 (1-12)
     */
    void send(unsigned long code, unsigned int bits, unsigned int freq, unsigned int protocol);

    /**
     * 设置重复发送次数
     * @param repeat 重复次数 (默认10)
     */
    void setRepeatTransmit(int repeat);

    /**
     * 是否正在发送
     */
    bool isSending() { return _sending; }

private:
    RCSwitch433 _rcSwitch433;   // 433MHz发送
    RCSwitch315 _rcSwitch315;   // 315MHz发送

    bool _sending;
    int _repeatCount;
};

#endif // RF_TRANSMITTER_H
