/**
 * @file RCSwitch315.cpp
 * @brief 315MHz RF信号接收库实现 (基于rc-switch修改)
 */

#include "RCSwitch315.h"
#include <FunctionalInterrupt.h>

// 独立的静态变量 (与RCSwitch分开)
static volatile unsigned long rc315NReceivedValue = 0;
static volatile unsigned int rc315NReceivedBitlength = 0;
static volatile unsigned int rc315NReceivedDelay = 0;
static volatile unsigned int rc315NReceivedProtocol = 0;
static int rc315NReceiveTolerance = 60;
static const unsigned int rc315NSeparationLimit = 4300;

static volatile unsigned int rc315Timings[RCSWITCH315_MAX_CHANGES];
static unsigned int rc315TimingsIndex = 0;

// 调试用计数器
static volatile unsigned long rc315InterruptCount = 0;
static volatile unsigned int rc315LastTimingsCount = 0;

// 协议定义 (与rc-switch完全一致)
static const RCSwitch315::Protocol rc315Proto[] = {
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

static const unsigned int rc315NumProto = sizeof(rc315Proto) / sizeof(rc315Proto[0]);

RCSwitch315::RCSwitch315() {
    nReceiverInterrupt = -1;
    nTransmitterPin = -1;
    nRepeatTransmit = 10;
    setProtocol(1);  // 默认协议1 (PT2262)
}

void RCSwitch315::setReceiveTolerance(int nPercent) {
    rc315NReceiveTolerance = nPercent;
}

void RCSwitch315::setProtocol(Protocol proto) {
    this->protocol = proto;
}

void RCSwitch315::setProtocol(int nProtocol) {
    if (nProtocol < 1 || nProtocol > (int)rc315NumProto) {
        nProtocol = 1;
    }
    this->protocol = rc315Proto[nProtocol - 1];
}

void RCSwitch315::setProtocol(int nProtocol, int nPulseLength) {
    setProtocol(nProtocol);
    this->protocol.pulseLength = nPulseLength;
}

void RCSwitch315::setPulseLength(int nPulseLength) {
    this->protocol.pulseLength = nPulseLength;
}

void RCSwitch315::setRepeatTransmit(int repeat) {
    this->nRepeatTransmit = repeat;
}

void RCSwitch315::enableReceive(int interrupt) {
    nReceiverInterrupt = interrupt;
    enableReceive();
}

void RCSwitch315::enableReceive() {
    if (nReceiverInterrupt != -1) {
        rc315NReceivedValue = 0;
        rc315NReceivedBitlength = 0;
        rc315TimingsIndex = 0;
        attachInterrupt(nReceiverInterrupt, handleInterrupt, CHANGE);
    }
}

void RCSwitch315::disableReceive() {
    if (nReceiverInterrupt != -1) {
        detachInterrupt(nReceiverInterrupt);
        nReceiverInterrupt = -1;
    }
}

bool RCSwitch315::available() {
    return rc315NReceivedValue != 0;
}

void RCSwitch315::resetAvailable() {
    rc315NReceivedValue = 0;
}

unsigned long RCSwitch315::getReceivedValue() {
    return rc315NReceivedValue;
}

unsigned int RCSwitch315::getReceivedBitlength() {
    return rc315NReceivedBitlength;
}

unsigned int RCSwitch315::getReceivedDelay() {
    return rc315NReceivedDelay;
}

unsigned int RCSwitch315::getReceivedProtocol() {
    return rc315NReceivedProtocol;
}

unsigned int* RCSwitch315::getReceivedRawdata() {
    return (unsigned int*)rc315Timings;
}

bool IRAM_ATTR RCSwitch315::receiveProtocol(const int p, unsigned int changeCount) {
    const Protocol &pro = rc315Proto[p - 1];

    unsigned long code = 0;
    const unsigned int syncLengthInPulses = ((pro.syncFactor.low) > (pro.syncFactor.high))
                                           ? (pro.syncFactor.low) : (pro.syncFactor.high);
    const unsigned int delay = rc315Timings[0] / syncLengthInPulses;
    const unsigned int delayTolerance = delay * rc315NReceiveTolerance / 100;

    // 同步位匹配
    const unsigned int firstDataTiming = (pro.invertedSignal) ? (2) : (1);

    for (unsigned int i = firstDataTiming; i < changeCount - 1; i += 2) {
        code <<= 1;
        if (abs((int)rc315Timings[i] - (int)(delay * pro.zero.high)) < delayTolerance &&
            abs((int)rc315Timings[i + 1] - (int)(delay * pro.zero.low)) < delayTolerance) {
            // 零位
        } else if (abs((int)rc315Timings[i] - (int)(delay * pro.one.high)) < delayTolerance &&
                   abs((int)rc315Timings[i + 1] - (int)(delay * pro.one.low)) < delayTolerance) {
            code |= 1;
        } else {
            return false;
        }
    }

    if (changeCount > 7) {
        rc315NReceivedValue = code;
        rc315NReceivedBitlength = (changeCount - 1) / 2;
        rc315NReceivedDelay = delay;
        rc315NReceivedProtocol = p;
        return true;
    }

    return false;
}

void IRAM_ATTR RCSwitch315::handleInterrupt() {
    static unsigned long lastTime = 0;

    rc315InterruptCount++;  // 调试：计数中断次数

    const unsigned long time = micros();
    const unsigned int duration = time - lastTime;

    if (duration > rc315NSeparationLimit) {
        // 长脉冲，可能是同步信号
        rc315LastTimingsCount = rc315TimingsIndex;  // 调试：记录时序数量

        if ((abs((int)duration - (int)rc315Timings[0]) < 200) ||
            (rc315TimingsIndex >= 7 && rc315TimingsIndex <= RCSWITCH315_MAX_CHANGES)) {
            // 检测所有协议
            for (unsigned int i = 1; i <= rc315NumProto; i++) {
                if (receiveProtocol(i, rc315TimingsIndex)) {
                    break;
                }
            }
        }
        rc315TimingsIndex = 0;
    }

    if (rc315TimingsIndex < RCSWITCH315_MAX_CHANGES) {
        rc315Timings[rc315TimingsIndex++] = duration;
    }

    lastTime = time;
}

// 调试功能实现
unsigned long RCSwitch315::getInterruptCount() {
    return rc315InterruptCount;
}

void RCSwitch315::resetInterruptCount() {
    rc315InterruptCount = 0;
    rc315LastTimingsCount = 0;
}

unsigned int RCSwitch315::getLastTimingsCount() {
    return rc315LastTimingsCount;
}

// ========== 发送功能实现 ==========

void RCSwitch315::enableTransmit(int pin) {
    this->nTransmitterPin = pin;
    pinMode(this->nTransmitterPin, OUTPUT);
    digitalWrite(this->nTransmitterPin, LOW);
}

void RCSwitch315::disableTransmit() {
    this->nTransmitterPin = -1;
}

void RCSwitch315::transmit(HighLow pulses) {
    uint8_t firstLogicLevel = (this->protocol.invertedSignal) ? LOW : HIGH;
    uint8_t secondLogicLevel = (this->protocol.invertedSignal) ? HIGH : LOW;

    digitalWrite(this->nTransmitterPin, firstLogicLevel);
    delayMicroseconds(this->protocol.pulseLength * pulses.high);
    digitalWrite(this->nTransmitterPin, secondLogicLevel);
    delayMicroseconds(this->protocol.pulseLength * pulses.low);
}

void RCSwitch315::send(unsigned long code, unsigned int length) {
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
