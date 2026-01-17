/**
 * @file RCSwitch315.h
 * @brief 315MHz RF信号收发库 (基于rc-switch修改)
 *
 * 这是RCSwitch库的复制版本，使用独立的类名和静态变量，
 * 可以与RCSwitch433同时使用，实现双频收发。
 */

#ifndef RCSwitch315_h
#define RCSwitch315_h

#include <Arduino.h>

// 最大信号变化次数
#define RCSWITCH315_MAX_CHANGES 67

class RCSwitch315 {
public:
    RCSwitch315();

    // ========== 接收功能 ==========
    void enableReceive(int interrupt);
    void enableReceive();
    void disableReceive();
    bool available();
    void resetAvailable();

    unsigned long getReceivedValue();
    unsigned int getReceivedBitlength();
    unsigned int getReceivedDelay();
    unsigned int getReceivedProtocol();
    unsigned int* getReceivedRawdata();

    void setReceiveTolerance(int nPercent);

    // ========== 发送功能 ==========
    void enableTransmit(int nTransmitterPin);
    void disableTransmit();
    void send(unsigned long code, unsigned int length);
    void setRepeatTransmit(int nRepeatTransmit);
    void setPulseLength(int nPulseLength);

    // 协议结构
    struct HighLow {
        uint8_t high;
        uint8_t low;
    };

    struct Protocol {
        uint16_t pulseLength;
        HighLow syncFactor;
        HighLow zero;
        HighLow one;
        bool invertedSignal;
    };

    void setProtocol(Protocol protocol);
    void setProtocol(int nProtocol);
    void setProtocol(int nProtocol, int nPulseLength);

    // 调试功能
    static unsigned long getInterruptCount();
    static void resetInterruptCount();
    static unsigned int getLastTimingsCount();

private:
    // 接收相关
    static void handleInterrupt();
    static bool receiveProtocol(const int p, unsigned int changeCount);
    int nReceiverInterrupt;

    // 发送相关
    void transmit(HighLow pulses);
    int nTransmitterPin;
    int nRepeatTransmit;
    Protocol protocol;
};

#endif
