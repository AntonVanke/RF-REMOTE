#include "SignalRxPage.h"
#include <Arduino.h>
#include <esp32-hal-log.h>

static const char* TAG = "SignalRx";

SignalRxPage::SignalRxPage(U8G2* u8g2, RFReceiver* receiver, SignalStorage* storage)
    : _u8g2(u8g2)
    , _receiver(receiver)
    , _storage(storage)
    , _hasSignal(false)
    , _signalExists(false)
    , _lastCode(0)
    , _lastCodeTime(0)
    , _debugMode(false)
    , _lastDebugUpdate(0)
{
    memset(&_currentSignal, 0, sizeof(_currentSignal));
    memset(_savedName, 0, sizeof(_savedName));
}

void SignalRxPage::enter() {
    ESP_LOGI(TAG, "进入: 信号接收页面");
    _hasSignal = false;
    _signalExists = false;
    _lastCode = 0;
    _lastCodeTime = 0;
    _debugMode = false;

    // 启动RF接收扫描
    _receiver->startScanning();

    // 重置调试计数器
    _receiver->resetDebugCounters();
}

void SignalRxPage::exit() {
    ESP_LOGI(TAG, "退出: 信号接收页面");
    // 停止RF接收
    _receiver->stopScanning();
}

bool SignalRxPage::update() {
    // 更新RF接收
    _receiver->update();

    // 检查是否有新信号
    if (_receiver->hasNewSignal()) {
        RFReceiver::Signal newSignal = _receiver->getLastSignal();
        unsigned long now = millis();

        // 防重复：同一编码在时间窗口内忽略
        if (newSignal.code == _lastCode && (now - _lastCodeTime) < DUPLICATE_WINDOW) {
            ESP_LOGD(TAG, "重复信号，忽略: %lu", newSignal.code);
            return false;  // 不更新界面
        }

        // 记录当前信号
        _currentSignal = newSignal;
        _lastCode = newSignal.code;
        _lastCodeTime = now;
        _hasSignal = true;

        ESP_LOGI(TAG, "收到信号: %dMHz 编码:%lu 协议:%s",
                 _currentSignal.freq,
                 _currentSignal.code,
                 RFReceiver::getProtocolName(_currentSignal.protocol));

        // 检查是否已存在
        if (_storage->signalExists(_currentSignal.code)) {
            _signalExists = true;
            ESP_LOGI(TAG, "信号已存在于存储中");
        } else {
            _signalExists = false;
            // 保存信号
            SignalStorage::StoredSignal stored;
            SignalStorage::generateName(_currentSignal.freq, _currentSignal.code,
                                        stored.name, sizeof(stored.name));
            stored.code = _currentSignal.code;
            stored.freq = _currentSignal.freq;
            stored.protocol = _currentSignal.protocol;
            stored.bits = _currentSignal.bits;

            if (_storage->saveSignal(stored)) {
                strncpy(_savedName, stored.name, sizeof(_savedName) - 1);
                ESP_LOGI(TAG, "信号已保存: %s", _savedName);
            } else {
                ESP_LOGE(TAG, "保存信号失败");
            }
        }

        return true;  // 需要重绘
    }

    return false;  // 无变化，不需要重绘
}

void SignalRxPage::draw() {
    if (_debugMode) {
        drawDebugInfo();
    } else if (_hasSignal) {
        drawSignalInfo();
    } else {
        drawWaiting();
    }
}

void SignalRxPage::drawWaiting() {
    // 等待信号状态
    _u8g2->setFont(u8g2_font_wqy12_t_gb2312);
    _u8g2->drawUTF8(0, 28, "等待信号...");

    // 显示已保存数量
    char countText[24];
    snprintf(countText, sizeof(countText), "已保存: %d", _storage->getSignalCount());
    _u8g2->drawUTF8(0, 44, countText);

    // 提示调试模式
    _u8g2->setFont(u8g2_font_6x10_tf);
    _u8g2->drawStr(0, 62, "[OK] Debug mode");
}

void SignalRxPage::drawSignalInfo() {
    // 第1行: 状态（已保存/已存在）
    _u8g2->setFont(u8g2_font_wqy12_t_gb2312);
    if (_signalExists) {
        _u8g2->drawUTF8(0, 28, "信号已存在");
    } else {
        char statusText[20];
        snprintf(statusText, sizeof(statusText), "%dMHz 已保存", _currentSignal.freq);
        _u8g2->drawUTF8(0, 28, statusText);
    }

    // 第2行: 编码
    _u8g2->setFont(u8g2_font_6x10_tf);
    char codeText[24];
    snprintf(codeText, sizeof(codeText), "Code: %lu", _currentSignal.code);
    _u8g2->drawStr(0, 40, codeText);

    // 第3行: 协议和位数
    char protoText[24];
    snprintf(protoText, sizeof(protoText), "%s %dbit",
             RFReceiver::getProtocolName(_currentSignal.protocol),
             _currentSignal.bits);
    _u8g2->drawStr(0, 52, protoText);

    // 第4行: 保存名称或频率
    if (_signalExists) {
        char freqText[16];
        snprintf(freqText, sizeof(freqText), "Freq: %dMHz", _currentSignal.freq);
        _u8g2->drawStr(0, 62, freqText);
    } else {
        _u8g2->drawStr(0, 62, _savedName);
    }
}

bool SignalRxPage::handleButton(ButtonEvent event) {
    switch (event) {
        case BTN_UP_LONG:
            ESP_LOGD(TAG, "按键: 上键长按 - 返回主菜单");
            exit();  // 退出前停止扫描
            return false;

        case BTN_UP_SHORT:
            ESP_LOGD(TAG, "按键: 上");
            return true;

        case BTN_DOWN_SHORT:
            ESP_LOGD(TAG, "按键: 下");
            return true;

        case BTN_OK_SHORT:
            // 切换调试模式
            _debugMode = !_debugMode;
            if (_debugMode) {
                _receiver->resetDebugCounters();  // 进入调试模式时重置计数器
                ESP_LOGI(TAG, "调试模式: 开启");
            } else {
                ESP_LOGI(TAG, "调试模式: 关闭");
            }
            return true;

        default:
            return true;
    }
}

void SignalRxPage::drawDebugInfo() {
    // 调试模式标题
    _u8g2->setFont(u8g2_font_6x10_tf);
    _u8g2->drawStr(0, 26, "== DEBUG MODE ==");

    // 显示433MHz中断计数和时序
    char line1[32];
    unsigned long irq433 = _receiver->get433InterruptCount();
    unsigned int tim433 = _receiver->get433TimingsCount();
    snprintf(line1, sizeof(line1), "433: IRQ=%lu T=%u", irq433, tim433);
    _u8g2->drawStr(0, 38, line1);

    // 显示315MHz中断计数和时序
    char line2[32];
    unsigned long irq315 = _receiver->get315InterruptCount();
    unsigned int tim315 = _receiver->get315TimingsCount();
    snprintf(line2, sizeof(line2), "315: IRQ=%lu T=%u", irq315, tim315);
    _u8g2->drawStr(0, 50, line2);

    // 提示
    _u8g2->drawStr(0, 62, "[OK] Exit debug");
}
