/**
 * @file RCSwitch433.h
 * @brief 433MHz RF信号收发库 (基于rc-switch修改)
 *
 * 这是RCSwitch库的本地版本，支持接收和发送功能
 * 可以获取中断计数和时序数量，用于诊断接收问题
 */

#ifndef RCSwitch433_h
#define RCSwitch433_h

#include <Arduino.h>

// 最大信号变化次数
#define RCSWITCH433_MAX_CHANGES 67

class RCSwitch433 {
public:
    RCSwitch433();

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
