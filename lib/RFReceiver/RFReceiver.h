/**
 * @file RFReceiver.h
 * @brief RF信号接收模块，支持433/315MHz同时监听
 *
 * 使用两个独立的库同时监听：
 * - RCSwitch433: 监听433MHz (RF_433_RX_PIN) - 本地库，支持调试
 * - RCSwitch315: 监听315MHz (RF_315_RX_PIN) - 本地库，支持调试
 */

#ifndef RF_RECEIVER_H
#define RF_RECEIVER_H

#include <Arduino.h>
#include "RCSwitch433.h"
#include "RCSwitch315.h"
#include "pin_config.h"

class RFReceiver {
public:
    // 接收到的信号结构
    struct Signal {
        unsigned long code;     // 编码值
        unsigned int protocol;  // 协议类型 (1=PT2262等)
        unsigned int bits;      // 位长度
        unsigned int freq;      // 频率 (433/315)
        unsigned long timestamp;// 接收时间戳
    };

    RFReceiver();

    /**
     * 初始化RF接收模块
     */
    void begin();

    /**
     * 每帧调用，检查是否有新信号
     */
    void update();

    /**
     * 是否有新信号（未被读取）
     */
    bool hasNewSignal();

    /**
     * 获取最后接收的信号，并清除新信号标志
     */
    Signal getLastSignal();

    /**
     * 获取协议名称
     * @param protocol 协议编号
     * @return 协议名称字符串
     */
    static const char* getProtocolName(unsigned int protocol);

    /**
     * 是否正在扫描
     */
    bool isScanning() { return _scanning; }

    /**
     * 启动扫描 (同时监听433和315MHz)
     */
    void startScanning();

    /**
     * 停止扫描
     */
    void stopScanning();

    // ========== 调试功能 ==========
    /**
     * 获取433MHz中断计数 (用于诊断接收问题)
     */
    unsigned long get433InterruptCount();

    /**
     * 获取315MHz中断计数 (用于诊断接收问题)
     */
    unsigned long get315InterruptCount();

    /**
     * 重置调试计数器
     */
    void resetDebugCounters();

    /**
     * 获取433MHz上次时序数量
     */
    unsigned int get433TimingsCount();

    /**
     * 获取315MHz上次时序数量
     */
    unsigned int get315TimingsCount();

private:
    RCSwitch433 _rcSwitch433;   // 433MHz接收 (本地库)
    RCSwitch315 _rcSwitch315;   // 315MHz接收 (本地库)

    Signal _lastSignal;
    bool _hasNewSignal;
    bool _scanning;

    /**
     * 检查433MHz是否有信号
     */
    void check433();

    /**
     * 检查315MHz是否有信号
     */
    void check315();
};

#endif // RF_RECEIVER_H
