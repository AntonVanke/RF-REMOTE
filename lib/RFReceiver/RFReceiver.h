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
    // 最小有效位数 (低于此值视为干扰信号，常见遥控器为24位)
    static const unsigned int MIN_VALID_BITS = 20;

    // 去重窗口时间 (ms) - 防止同一信号在两个频率上重复识别
    static const unsigned long DUPLICATE_WINDOW_MS = 100;

    // 接收冷却时间 (ms) - 收到有效信号后，忽略所有信号的时间
    static const unsigned long RECEIVE_COOLDOWN_MS = 500;

    // 接收到的信号结构
    struct Signal {
        unsigned long code;     // 编码值
        unsigned int protocol;  // 协议类型 (1=PT2262等)
        unsigned int bits;      // 位长度
        unsigned int freq;      // 频率 (433/315)
        unsigned int pulseLength; // 脉宽 (微秒)
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

    // 去重用变量
    unsigned long _lastReceivedCode;    // 上次接收的编码
    unsigned long _lastReceivedTime;    // 上次接收的时间

    // 冷却时间变量
    unsigned long _lastValidSignalTime; // 上次收到有效信号的时间

    /**
     * 检查是否在冷却期内
     * @return true=冷却中, false=可以接收
     */
    bool isInCooldown();

    /**
     * 检查433MHz是否有信号
     */
    void check433();

    /**
     * 检查315MHz是否有信号
     */
    void check315();

    /**
     * 验证信号是否有效 (过滤干扰)
     * @param code 编码值
     * @param bits 位数
     * @return true=有效信号, false=干扰信号
     */
    bool isValidSignal(unsigned long code, unsigned int bits);

    /**
     * 检查是否为重复信号 (防止433/315混淆)
     * @param code 编码值
     * @return true=重复信号, false=新信号
     */
    bool isDuplicateSignal(unsigned long code);
};

#endif // RF_RECEIVER_H
