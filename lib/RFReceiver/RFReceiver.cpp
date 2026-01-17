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
    , _lastReceivedCode(0)
    , _lastReceivedTime(0)
    , _lastValidSignalTime(0)
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

bool RFReceiver::isValidSignal(unsigned long code, unsigned int bits) {
    // 1. 过滤短位数信号
    if (bits < MIN_VALID_BITS) {
        return false;
    }

    // 2. 过滤 2^n-1 位的信号 (通常是噪声: 3,7,15,31位)
    // 检查 bits+1 是否为2的幂
    unsigned int bitsPlus1 = bits + 1;
    if ((bitsPlus1 & (bitsPlus1 - 1)) == 0) {
        return false;
    }

    // 3. 过滤全1的编码 (噪声特征)
    unsigned long maxCode = (1UL << bits) - 1;
    if (code == maxCode) {
        return false;
    }

    return true;
}

bool RFReceiver::isDuplicateSignal(unsigned long code) {
    unsigned long now = millis();
    // 在去重窗口内收到相同编码，视为重复
    if (code == _lastReceivedCode && (now - _lastReceivedTime) < DUPLICATE_WINDOW_MS) {
        return true;
    }
    // 更新最后接收记录
    _lastReceivedCode = code;
    _lastReceivedTime = now;
    return false;
}

bool RFReceiver::isInCooldown() {
    unsigned long now = millis();
    if ((now - _lastValidSignalTime) < RECEIVE_COOLDOWN_MS) {
        return true;
    }
    return false;
}

void RFReceiver::check433() {
    if (_rcSwitch433.available()) {
        unsigned long code = _rcSwitch433.getReceivedValue();
        unsigned int bits = _rcSwitch433.getReceivedBitlength();

        // 冷却期内忽略所有信号
        if (isInCooldown()) {
            _rcSwitch433.resetAvailable();
            return;
        }

        if (code != 0) {
            // 过滤无效信号
            if (!isValidSignal(code, bits)) {
                ESP_LOGD(TAG, "433MHz忽略无效信号: 编码:%lu 位数:%d", code, bits);
                _rcSwitch433.resetAvailable();
                return;
            }

            // 过滤重复信号 (防止433/315混淆)
            if (isDuplicateSignal(code)) {
                ESP_LOGD(TAG, "433MHz忽略重复信号: 编码:%lu", code);
                _rcSwitch433.resetAvailable();
                return;
            }

            _lastSignal.code = code;
            _lastSignal.protocol = _rcSwitch433.getReceivedProtocol();
            _lastSignal.bits = bits;
            _lastSignal.freq = 433;
            _lastSignal.pulseLength = _rcSwitch433.getReceivedDelay();
            _lastSignal.timestamp = millis();
            _hasNewSignal = true;
            _lastValidSignalTime = millis();  // 更新冷却时间

            ESP_LOGI(TAG, "收到433MHz信号! 编码:%lu 协议:%d 位数:%d 脉宽:%dus",
                     _lastSignal.code,
                     _lastSignal.protocol,
                     _lastSignal.bits,
                     _lastSignal.pulseLength);
        }

        _rcSwitch433.resetAvailable();
    }
}

void RFReceiver::check315() {
    if (_rcSwitch315.available()) {
        unsigned long code = _rcSwitch315.getReceivedValue();
        unsigned int bits = _rcSwitch315.getReceivedBitlength();

        // 冷却期内忽略所有信号
        if (isInCooldown()) {
            _rcSwitch315.resetAvailable();
            return;
        }

        if (code != 0) {
            // 过滤无效信号
            if (!isValidSignal(code, bits)) {
                ESP_LOGD(TAG, "315MHz忽略无效信号: 编码:%lu 位数:%d", code, bits);
                _rcSwitch315.resetAvailable();
                return;
            }

            // 过滤重复信号 (防止433/315混淆)
            if (isDuplicateSignal(code)) {
                ESP_LOGD(TAG, "315MHz忽略重复信号: 编码:%lu", code);
                _rcSwitch315.resetAvailable();
                return;
            }

            _lastSignal.code = code;
            _lastSignal.protocol = _rcSwitch315.getReceivedProtocol();
            _lastSignal.bits = bits;
            _lastSignal.freq = 315;
            _lastSignal.pulseLength = _rcSwitch315.getReceivedDelay();
            _lastSignal.timestamp = millis();
            _hasNewSignal = true;
            _lastValidSignalTime = millis();  // 更新冷却时间

            ESP_LOGI(TAG, "收到315MHz信号! 编码:%lu 协议:%d 位数:%d 脉宽:%dus",
                     _lastSignal.code,
                     _lastSignal.protocol,
                     _lastSignal.bits,
                     _lastSignal.pulseLength);
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
