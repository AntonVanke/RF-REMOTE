#include "SignalTxPage.h"
#include <Arduino.h>
#include <esp32-hal-log.h>

static const char* TAG = "SignalTx";

SignalTxPage::SignalTxPage(U8G2* u8g2, SignalStorage* storage, RFTransmitter* transmitter)
    : _u8g2(u8g2)
    , _storage(storage)
    , _transmitter(transmitter)
    , _signalCount(0)
    , _selectedIndex(0)
    , _scrollOffset(0)
    , _sending(false)
    , _sendStartTime(0)
{
}

void SignalTxPage::enter() {
    ESP_LOGI(TAG, "进入: 发送模式页面");
    _selectedIndex = 0;
    _scrollOffset = 0;
    _sending = false;
    loadSignals();
}

void SignalTxPage::exit() {
    ESP_LOGI(TAG, "退出: 发送模式页面");
}

void SignalTxPage::loadSignals() {
    _signalCount = _storage->loadSignals(_signals, MAX_DISPLAY_SIGNALS);
    ESP_LOGI(TAG, "加载信号: %d 个", _signalCount);
}

bool SignalTxPage::update() {
    // 如果正在显示发送状态，检查是否超时
    if (_sending && (millis() - _sendStartTime > SEND_DISPLAY_TIME)) {
        _sending = false;
        return true;  // 需要重绘
    }
    return false;
}

void SignalTxPage::draw() {
    if (_sending) {
        drawSendingStatus();
    } else if (_signalCount == 0) {
        drawEmptyMessage();
    } else {
        drawSignalList();
    }
}

void SignalTxPage::drawEmptyMessage() {
    _u8g2->setFont(u8g2_font_wqy12_t_gb2312);
    _u8g2->drawUTF8(10, 32, "没有保存的信号");

    _u8g2->setFont(u8g2_font_6x10_tf);
    _u8g2->drawStr(10, 48, "Go to Receive mode");
    _u8g2->drawStr(10, 60, "to capture signals");
}

void SignalTxPage::drawSignalList() {
    _u8g2->setFont(u8g2_font_6x10_tf);

    // 计算显示范围
    int startIdx = _scrollOffset;
    int endIdx = min(_scrollOffset + ITEMS_PER_PAGE, _signalCount);

    // 显示信号列表
    int y = 28;
    for (int i = startIdx; i < endIdx; i++) {
        // 选中标记
        if (i == _selectedIndex) {
            _u8g2->drawStr(0, y, ">");
        }

        // 信号名称 (频率_编码)
        char line[32];
        snprintf(line, sizeof(line), "%dM %lu",
                 _signals[i].freq, _signals[i].code);
        _u8g2->drawStr(10, y, line);

        y += 12;
    }

    // 显示滚动指示器
    if (_signalCount > ITEMS_PER_PAGE) {
        char countText[16];
        snprintf(countText, sizeof(countText), "%d/%d", _selectedIndex + 1, _signalCount);
        _u8g2->drawStr(90, 62, countText);
    }

    // 底部提示
    _u8g2->drawStr(0, 62, "[OK] Send");
}

void SignalTxPage::drawSendingStatus() {
    _u8g2->setFont(u8g2_font_wqy12_t_gb2312);
    _u8g2->drawUTF8(30, 32, "发送中...");

    // 显示正在发送的信号信息
    _u8g2->setFont(u8g2_font_6x10_tf);
    char info[32];
    snprintf(info, sizeof(info), "%dMHz %lu",
             _signals[_selectedIndex].freq,
             _signals[_selectedIndex].code);
    _u8g2->drawStr(20, 50, info);
}

void SignalTxPage::sendSelectedSignal() {
    if (_signalCount == 0 || _selectedIndex >= _signalCount) {
        return;
    }

    SignalStorage::StoredSignal& sig = _signals[_selectedIndex];

    ESP_LOGI(TAG, "发送信号: %s 编码:%lu 频率:%dMHz 协议:%d 位数:%d",
             sig.name, sig.code, sig.freq, sig.protocol, sig.bits);

    _sending = true;
    _sendStartTime = millis();

    // 发送信号
    _transmitter->send(sig.code, sig.bits, sig.freq, sig.protocol);
}

bool SignalTxPage::handleButton(ButtonEvent event) {
    // 发送中不响应按键
    if (_sending) {
        return true;
    }

    switch (event) {
        case BTN_UP_LONG:
            ESP_LOGD(TAG, "按键: 上键长按 - 返回主菜单");
            return false;

        case BTN_UP_SHORT:
            ESP_LOGD(TAG, "按键: 上");
            if (_signalCount > 0 && _selectedIndex > 0) {
                _selectedIndex--;
                // 滚动调整
                if (_selectedIndex < _scrollOffset) {
                    _scrollOffset = _selectedIndex;
                }
            }
            return true;

        case BTN_DOWN_SHORT:
            ESP_LOGD(TAG, "按键: 下");
            if (_signalCount > 0 && _selectedIndex < _signalCount - 1) {
                _selectedIndex++;
                // 滚动调整
                if (_selectedIndex >= _scrollOffset + ITEMS_PER_PAGE) {
                    _scrollOffset = _selectedIndex - ITEMS_PER_PAGE + 1;
                }
            }
            return true;

        case BTN_OK_SHORT:
            ESP_LOGD(TAG, "按键: 确认 - 发送信号");
            sendSelectedSignal();
            return true;

        default:
            return true;
    }
}
