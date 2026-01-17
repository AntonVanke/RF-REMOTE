/**
 * @file RCSwitch433.cpp
 * @brief 433MHz RF信号收发库实现 (基于rc-switch修改)
 */

#include "RCSwitch433.h"
#include <FunctionalInterrupt.h>

// 独立的静态变量
static volatile unsigned long rc433NReceivedValue = 0;
static volatile unsigned int rc433NReceivedBitlength = 0;
static volatile unsigned int rc433NReceivedDelay = 0;
static volatile unsigned int rc433NReceivedProtocol = 0;
static int rc433NReceiveTolerance = 60;
static const unsigned int rc433NSeparationLimit = 4300;

static volatile unsigned int rc433Timings[RCSWITCH433_MAX_CHANGES];
static unsigned int rc433TimingsIndex = 0;

// 调试用计数器
static volatile unsigned long rc433InterruptCount = 0;
static volatile unsigned int rc433LastTimingsCount = 0;

// 协议定义 (与rc-switch完全一致)
static const RCSwitch433::Protocol rc433Proto[] = {
    { 350, {  1, 31 }, {  1,  3 }, {  3,  1 }, false },  // 1: PT2262
    { 650, {  1, 10 }, {  1,  2 }, {  2,  1 }, false },  // 2: PT2260
    { 100, { 30, 71 }, {  4, 11 }, {  9,  6 }, false },  // 3: EV1527
    { 380, {  1,  6 }, {  1,  3 }, {  3,  1 }, false },  // 4: HT6P20B
    { 500, {  6, 14 }, {  1,  2 }, {  2,  1 }, false },  // 5: SC5262
    { 450, { 23,  1 }, {  1,  2 }, {  2,  1 }, true  },  // 6: HT6P20B
    { 150, {  2, 62 }, {  1,  6 }, {  6,  1 }, false },  // 7: HS2303-PT
    { 200, {  3, 130}, {  7, 16 }, {  3, 16 }, false },  // 8: Conrad RS-200 RX
    { 200, { 130, 7 }, { 16,  7 }, { 16,  3 }, true  },  // 9: Conrad RS-200 TX
    { 365, { 18,  1 }, {  3,  1 }, {  1,  3 }, true  },  // 10: 1ByOne Doorbell
    { 270, { 36,  1 }, {  1,  2 }, {  2,  1 }, true  },  // 11: HT12E
    { 320, { 36,  1 }, {  1,  2 }, {  2,  1 }, true  },  // 12: SM5212
};

static const unsigned int rc433NumProto = sizeof(rc433Proto) / sizeof(rc433Proto[0]);

RCSwitch433::RCSwitch433() {
    nReceiverInterrupt = -1;
    nTransmitterPin = -1;
    nRepeatTransmit = 10;
    setProtocol(1);  // 默认协议1 (PT2262)
}

void RCSwitch433::setReceiveTolerance(int nPercent) {
    rc433NReceiveTolerance = nPercent;
}

void RCSwitch433::setProtocol(Protocol proto) {
    this->protocol = proto;
}

void RCSwitch433::setProtocol(int nProtocol) {
    if (nProtocol < 1 || nProtocol > (int)rc433NumProto) {
        nProtocol = 1;
    }
    this->protocol = rc433Proto[nProtocol - 1];
}

void RCSwitch433::setProtocol(int nProtocol, int nPulseLength) {
    setProtocol(nProtocol);
    this->protocol.pulseLength = nPulseLength;
}

void RCSwitch433::setPulseLength(int nPulseLength) {
    this->protocol.pulseLength = nPulseLength;
}

void RCSwitch433::setRepeatTransmit(int repeat) {
    this->nRepeatTransmit = repeat;
}

void RCSwitch433::enableReceive(int interrupt) {
    nReceiverInterrupt = interrupt;
    enableReceive();
}

void RCSwitch433::enableReceive() {
    if (nReceiverInterrupt != -1) {
        rc433NReceivedValue = 0;
        rc433NReceivedBitlength = 0;
        rc433TimingsIndex = 0;
        attachInterrupt(nReceiverInterrupt, handleInterrupt, CHANGE);
    }
}

void RCSwitch433::disableReceive() {
    if (nReceiverInterrupt != -1) {
        detachInterrupt(nReceiverInterrupt);
        nReceiverInterrupt = -1;
    }
}

bool RCSwitch433::available() {
    return rc433NReceivedValue != 0;
}

void RCSwitch433::resetAvailable() {
    rc433NReceivedValue = 0;
}

unsigned long RCSwitch433::getReceivedValue() {
    return rc433NReceivedValue;
}

unsigned int RCSwitch433::getReceivedBitlength() {
    return rc433NReceivedBitlength;
}

unsigned int RCSwitch433::getReceivedDelay() {
    return rc433NReceivedDelay;
}

unsigned int RCSwitch433::getReceivedProtocol() {
    return rc433NReceivedProtocol;
}

unsigned int* RCSwitch433::getReceivedRawdata() {
    return (unsigned int*)rc433Timings;
}

bool IRAM_ATTR RCSwitch433::receiveProtocol(const int p, unsigned int changeCount) {
    const Protocol &pro = rc433Proto[p - 1];

    unsigned long code = 0;
    const unsigned int syncLengthInPulses = ((pro.syncFactor.low) > (pro.syncFactor.high))
                                           ? (pro.syncFactor.low) : (pro.syncFactor.high);
    const unsigned int delay = rc433Timings[0] / syncLengthInPulses;
    const unsigned int delayTolerance = delay * rc433NReceiveTolerance / 100;

    // 同步位匹配
    const unsigned int firstDataTiming = (pro.invertedSignal) ? (2) : (1);

    for (unsigned int i = firstDataTiming; i < changeCount - 1; i += 2) {
        code <<= 1;
        if (abs((int)rc433Timings[i] - (int)(delay * pro.zero.high)) < delayTolerance &&
            abs((int)rc433Timings[i + 1] - (int)(delay * pro.zero.low)) < delayTolerance) {
            // 零位
        } else if (abs((int)rc433Timings[i] - (int)(delay * pro.one.high)) < delayTolerance &&
                   abs((int)rc433Timings[i + 1] - (int)(delay * pro.one.low)) < delayTolerance) {
            code |= 1;
        } else {
            return false;
        }
    }

    if (changeCount > 7) {
        rc433NReceivedValue = code;
        rc433NReceivedBitlength = (changeCount - 1) / 2;
        rc433NReceivedDelay = delay;
        rc433NReceivedProtocol = p;
        return true;
    }

    return false;
}

void IRAM_ATTR RCSwitch433::handleInterrupt() {
    static unsigned long lastTime = 0;

    rc433InterruptCount++;  // 调试：计数中断次数

    const unsigned long time = micros();
    const unsigned int duration = time - lastTime;

    if (duration > rc433NSeparationLimit) {
        // 长脉冲，可能是同步信号
        rc433LastTimingsCount = rc433TimingsIndex;  // 调试：记录时序数量

        if ((abs((int)duration - (int)rc433Timings[0]) < 200) ||
            (rc433TimingsIndex >= 7 && rc433TimingsIndex <= RCSWITCH433_MAX_CHANGES)) {
            // 检测所有协议
            for (unsigned int i = 1; i <= rc433NumProto; i++) {
                if (receiveProtocol(i, rc433TimingsIndex)) {
                    break;
                }
            }
        }
        rc433TimingsIndex = 0;
    }

    if (rc433TimingsIndex < RCSWITCH433_MAX_CHANGES) {
        rc433Timings[rc433TimingsIndex++] = duration;
    }

    lastTime = time;
}

// 调试功能实现
unsigned long RCSwitch433::getInterruptCount() {
    return rc433InterruptCount;
}

void RCSwitch433::resetInterruptCount() {
    rc433InterruptCount = 0;
    rc433LastTimingsCount = 0;
}

unsigned int RCSwitch433::getLastTimingsCount() {
    return rc433LastTimingsCount;
}

// ========== 发送功能实现 ==========

void RCSwitch433::enableTransmit(int pin) {
    this->nTransmitterPin = pin;
    pinMode(this->nTransmitterPin, OUTPUT);
    digitalWrite(this->nTransmitterPin, LOW);
}

void RCSwitch433::disableTransmit() {
    this->nTransmitterPin = -1;
}

void RCSwitch433::transmit(HighLow pulses) {
    uint8_t firstLogicLevel = (this->protocol.invertedSignal) ? LOW : HIGH;
    uint8_t secondLogicLevel = (this->protocol.invertedSignal) ? HIGH : LOW;

    digitalWrite(this->nTransmitterPin, firstLogicLevel);
    delayMicroseconds(this->protocol.pulseLength * pulses.high);
    digitalWrite(this->nTransmitterPin, secondLogicLevel);
    delayMicroseconds(this->protocol.pulseLength * pulses.low);
}

void RCSwitch433::send(unsigned long code, unsigned int length) {
    if (this->nTransmitterPin == -1) return;

    // 禁用接收中断，避免干扰
    int savedInterrupt = this->nReceiverInterrupt;
    if (savedInterrupt != -1) {
        this->disableReceive();
    }

    for (int nRepeat = 0; nRepeat < nRepeatTransmit; nRepeat++) {
        // 发送数据位
        for (int i = length - 1; i >= 0; i--) {
            if (code & (1UL << i)) {
                this->transmit(protocol.one);
            } else {
                this->transmit(protocol.zero);
            }
        }
        // 发送同步位
        this->transmit(protocol.syncFactor);
    }

    // 确保发送结束后为低电平
    digitalWrite(this->nTransmitterPin, LOW);

    // 恢复接收中断
    if (savedInterrupt != -1) {
        this->enableReceive(savedInterrupt);
    }
}
