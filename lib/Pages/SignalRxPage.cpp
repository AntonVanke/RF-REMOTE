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

    // 启动RF接收扫描
    _receiver->startScanning();
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
            stored.pulseLength = _currentSignal.pulseLength;

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
    if (_hasSignal) {
        drawSignalInfo();
    } else {
        drawWaiting();
    }
}

void SignalRxPage::drawWaiting() {
    // 等待信号状态 - 居中显示
    _u8g2->setFont(u8g2_font_wqy12_t_gb2312);

    // 显示等待中
    _u8g2->drawUTF8(30, 36, "等待信号...");

    // 显示已保存数量
    char countText[24];
    snprintf(countText, sizeof(countText), "已保存: %d/50", _storage->getSignalCount());
    _u8g2->drawUTF8(24, 52, countText);
}

void SignalRxPage::drawSignalInfo() {
    /*
     * 布局 (内容区 Y: 16-63, 共48像素高):
     * +------------------------------------------+
     * | 433MHz   | 4269192           | 已 |      | Y=28
     * | PT2262   | 0x412488          | 保 |      | Y=40
     * | 24b      | 信号:85           | 存 |      | Y=52
     * +------------------------------------------+
     *
     * 右侧竖排显示状态
     */

    _u8g2->setFont(u8g2_font_6x10_tf);

    // 第1行 Y=28
    // 左: 频率
    char freqText[10];
    snprintf(freqText, sizeof(freqText), "%dMHz", _currentSignal.freq);
    _u8g2->drawStr(0, 28, freqText);

    // 中: 十进制编码
    char codeText[12];
    snprintf(codeText, sizeof(codeText), "%lu", _currentSignal.code);
    _u8g2->drawStr(48, 28, codeText);

    // 第2行 Y=40
    // 左: 协议
    _u8g2->drawStr(0, 40, RFReceiver::getProtocolName(_currentSignal.protocol));

    // 中: 十六进制编码
    char hexText[12];
    snprintf(hexText, sizeof(hexText), "0x%lX", _currentSignal.code);
    _u8g2->drawStr(48, 40, hexText);

    // 第3行 Y=52
    // 左: 位数
    char bitsText[8];
    snprintf(bitsText, sizeof(bitsText), "%db", _currentSignal.bits);
    _u8g2->drawStr(0, 52, bitsText);

    // 中: 脉宽
    char pulseText[12];
    snprintf(pulseText, sizeof(pulseText), "%dus", _currentSignal.pulseLength);
    _u8g2->drawStr(48, 52, pulseText);

    // 右侧竖排显示状态 (使用中文字体)
    _u8g2->setFont(u8g2_font_wqy12_t_gb2312);
    if (_signalExists) {
        _u8g2->drawUTF8(116, 28, "已");
        _u8g2->drawUTF8(116, 42, "存");
        _u8g2->drawUTF8(116, 56, "在");
    } else {
        _u8g2->drawUTF8(116, 28, "已");
        _u8g2->drawUTF8(116, 42, "保");
        _u8g2->drawUTF8(116, 56, "存");
    }
}

bool SignalRxPage::handleButton(ButtonEvent event) {
    switch (event) {
        case BTN_UP_LONG:
            ESP_LOGD(TAG, "按键: 上键长按 - 返回主菜单");
            exit();  // 退出前停止扫描
            return false;

        case BTN_UP_SHORT:
        case BTN_DOWN_SHORT:
        case BTN_OK_SHORT:
        default:
            return true;
    }
}
