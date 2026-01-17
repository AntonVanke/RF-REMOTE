/**
 * @file RFReceiver.cpp
 * @brief RF信号接收模块实现 - 同时监听433/315MHz
 *
 * 使用两个独立的库：
 * - RCSwitch433: 433MHz (中断方式，本地库)
 * - RCSwitch315: 315MHz (中断方式，本地库)
 */

#include "RFReceiver.h"
#include <esp32-hal-log.h>

static const char* TAG = "RFReceiver";

// 协议名称映射
static const char* PROTOCOL_NAMES[] = {
    "Unknown",    // 0
    "PT2262",     // 1
    "PT2260",     // 2
    "EV1527",     // 3
    "HT6P20B",    // 4
    "SC5262",     // 5
    "HT12E",      // 6
    "HS2303-PT",  // 7
};
static const int PROTOCOL_COUNT = sizeof(PROTOCOL_NAMES) / sizeof(PROTOCOL_NAMES[0]);

RFReceiver::RFReceiver()
    : _hasNewSignal(false)
    , _scanning(false)
{
    memset(&_lastSignal, 0, sizeof(_lastSignal));
}

void RFReceiver::begin() {
    ESP_LOGI(TAG, "初始化RF接收模块...");
    ESP_LOGI(TAG, "433MHz RX引脚: %d (RCSwitch433)", RF_433_RX_PIN);
    ESP_LOGI(TAG, "315MHz RX引脚: %d (RCSwitch315)", RF_315_RX_PIN);

    // 初始化引脚为输入
    pinMode(RF_433_RX_PIN, INPUT);
    pinMode(RF_315_RX_PIN, INPUT);

    // 设置接收容差 (默认60%, 提高到80%增加兼容性)
    _rcSwitch433.setReceiveTolerance(80);
    _rcSwitch315.setReceiveTolerance(80);
    ESP_LOGI(TAG, "接收容差设置为 80%%");

    ESP_LOGI(TAG, "RF接收模块初始化完成 (双频同时监听)");
}

void RFReceiver::startScanning() {
    if (_scanning) return;

    ESP_LOGI(TAG, "开始扫描 (同时监听 433MHz + 315MHz)...");
    _scanning = true;

    // 同时启用两个接收器
    _rcSwitch433.enableReceive(digitalPinToInterrupt(RF_433_RX_PIN));
    _rcSwitch315.enableReceive(digitalPinToInterrupt(RF_315_RX_PIN));

    ESP_LOGI(TAG, "双频接收已启用");
}

void RFReceiver::stopScanning() {
    if (!_scanning) return;

    ESP_LOGI(TAG, "停止扫描");
    _scanning = false;

    // 禁用两个接收器
    _rcSwitch433.disableReceive();
    _rcSwitch315.disableReceive();
}

void RFReceiver::update() {
    if (!_scanning) return;

    // 检查433MHz
    check433();

    // 检查315MHz
    check315();
}

void RFReceiver::check433() {
    if (_rcSwitch433.available()) {
        unsigned long code = _rcSwitch433.getReceivedValue();

        if (code != 0) {
            _lastSignal.code = code;
            _lastSignal.protocol = _rcSwitch433.getReceivedProtocol();
            _lastSignal.bits = _rcSwitch433.getReceivedBitlength();
            _lastSignal.freq = 433;
            _lastSignal.timestamp = millis();
            _hasNewSignal = true;

            ESP_LOGI(TAG, "收到433MHz信号! 编码:%lu 协议:%d 位数:%d",
                     _lastSignal.code,
                     _lastSignal.protocol,
                     _lastSignal.bits);
        }

        _rcSwitch433.resetAvailable();
    }
}

void RFReceiver::check315() {
    if (_rcSwitch315.available()) {
        unsigned long code = _rcSwitch315.getReceivedValue();

        if (code != 0) {
            _lastSignal.code = code;
            _lastSignal.protocol = _rcSwitch315.getReceivedProtocol();
            _lastSignal.bits = _rcSwitch315.getReceivedBitlength();
            _lastSignal.freq = 315;
            _lastSignal.timestamp = millis();
            _hasNewSignal = true;

            ESP_LOGI(TAG, "收到315MHz信号! 编码:%lu 协议:%d 位数:%d",
                     _lastSignal.code,
                     _lastSignal.protocol,
                     _lastSignal.bits);
        }

        _rcSwitch315.resetAvailable();
    }
}

bool RFReceiver::hasNewSignal() {
    return _hasNewSignal;
}

RFReceiver::Signal RFReceiver::getLastSignal() {
    _hasNewSignal = false;
    return _lastSignal;
}

const char* RFReceiver::getProtocolName(unsigned int protocol) {
    if (protocol < PROTOCOL_COUNT) {
        return PROTOCOL_NAMES[protocol];
    }
    return "Unknown";
}

// ========== 调试功能实现 ==========

unsigned long RFReceiver::get433InterruptCount() {
    return RCSwitch433::getInterruptCount();
}

unsigned long RFReceiver::get315InterruptCount() {
    return RCSwitch315::getInterruptCount();
}

void RFReceiver::resetDebugCounters() {
    RCSwitch433::resetInterruptCount();
    RCSwitch315::resetInterruptCount();
    ESP_LOGD(TAG, "调试计数器已重置 (433MHz + 315MHz)");
}

unsigned int RFReceiver::get433TimingsCount() {
    return RCSwitch433::getLastTimingsCount();
}

unsigned int RFReceiver::get315TimingsCount() {
    return RCSwitch315::getLastTimingsCount();
}
