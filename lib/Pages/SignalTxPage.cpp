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
    , _arrowRight(true)
    , _editMode(false)
    , _editingDigit(false)
    , _cursorPos(0)
    , _editCode(0)
    , _digitCount(1)
{
}

void SignalTxPage::enter() {
    ESP_LOGI(TAG, "进入: 发送模式页面");
    _selectedIndex = 0;
    _scrollOffset = 0;
    _arrowRight = true;
    _editMode = false;
    loadSignals();
}

void SignalTxPage::exit() {
    ESP_LOGI(TAG, "退出: 发送模式页面");
    _editMode = false;
}

const char* SignalTxPage::getTitle() {
    if (_editMode) {
        if (_editingDigit) {
            return "编辑数值";
        }
        return "选择位置";
    }
    return "发送模式";
}

void SignalTxPage::loadSignals() {
    _signalCount = _storage->loadSignals(_signals, MAX_DISPLAY_SIGNALS);
    ESP_LOGI(TAG, "加载信号: %d 个", _signalCount);
}

bool SignalTxPage::update() {
    return false;
}

void SignalTxPage::draw() {
    if (_editMode) {
        drawEditMode();
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
        // 格式: "1    433M xxx" 或 "2 >  433M xxx" (选中项)
        char line[32];
        if (i == _selectedIndex) {
            // 选中项: 显示 > 或 <
            snprintf(line, sizeof(line), "%d %c %dM %lu",
                     i + 1, _arrowRight ? '>' : '<',
                     _signals[i].freq, _signals[i].code);
        } else {
            // 未选中项: 空格占位
            snprintf(line, sizeof(line), "%d   %dM %lu",
                     i + 1, _signals[i].freq, _signals[i].code);
        }
        _u8g2->drawStr(0, y, line);

        y += 12;
    }

    // 显示滚动指示器
    if (_signalCount > ITEMS_PER_PAGE) {
        char countText[16];
        snprintf(countText, sizeof(countText), "%d/%d", _selectedIndex + 1, _signalCount);
        _u8g2->drawStr(90, 62, countText);
    }

    // 底部提示
    _u8g2->drawStr(0, 62, "[OK]Send [+]Edit");
}

void SignalTxPage::drawEditMode() {
    // 使用更大的字体显示数字
    _u8g2->setFont(u8g2_font_logisoso16_tn);  // 16像素高数字字体

    // 编码值，固定位数显示（带前导零）
    char codeStr[16];
    snprintf(codeStr, sizeof(codeStr), "%0*lu", _digitCount, _editCode);

    // 计算居中位置 (该字体数字宽度约10像素)
    int digitWidth = 12;
    int totalWidth = _digitCount * digitWidth;
    int startX = (128 - totalWidth) / 2;

    int x = startX;
    int y = 38;

    for (int i = 0; i < _digitCount; i++) {
        char digit[2] = { codeStr[i], '\0' };

        bool isSelected = (_cursorPos == i);
        bool isEditing = isSelected && _editingDigit;

        if (isEditing) {
            // 正在编辑: 反色显示
            _u8g2->drawBox(x - 1, y - 14, digitWidth, 18);
            _u8g2->setDrawColor(0);
            _u8g2->drawStr(x, y, digit);
            _u8g2->setDrawColor(1);
        } else if (isSelected) {
            // 选中但未编辑: 下划线
            _u8g2->drawStr(x, y, digit);
            _u8g2->drawHLine(x - 1, y + 2, digitWidth);
        } else {
            // 普通显示
            _u8g2->drawStr(x, y, digit);
        }
        x += digitWidth;
    }

    // 底部: 只显示删除按钮，居中
    int btnY = 60;
    int delX = (128 - 32) / 2;
    bool delSelected = (_cursorPos == _digitCount);
    if (delSelected) {
        _u8g2->drawFrame(delX, btnY - 10, 32, 13);
    }
    _u8g2->setFont(u8g2_font_wqy12_t_gb2312);
    _u8g2->drawUTF8(delX + 4, btnY, "删除");
}

void SignalTxPage::enterEditMode() {
    if (_signalCount == 0 || _selectedIndex >= _signalCount) {
        return;
    }

    _editMode = true;
    _editingDigit = false;
    _editCode = _signals[_selectedIndex].code;
    _digitCount = calcDigitCount(_editCode);
    _cursorPos = 0;

    ESP_LOGI(TAG, "进入编辑模式: 编码=%lu, 位数=%d", _editCode, _digitCount);
}

void SignalTxPage::exitEditMode() {
    _editMode = false;
    _editingDigit = false;
    ESP_LOGI(TAG, "退出编辑模式");
}

void SignalTxPage::sendSelectedSignal() {
    if (_signalCount == 0 || _selectedIndex >= _signalCount) {
        return;
    }

    SignalStorage::StoredSignal& sig = _signals[_selectedIndex];

    ESP_LOGI(TAG, "发送信号: %s 编码:%lu 频率:%dMHz 协议:%d 位数:%d 脉宽:%dus",
             sig.name, sig.code, sig.freq, sig.protocol, sig.bits, sig.pulseLength);

    // 切换箭头方向 (> <-> <)
    _arrowRight = !_arrowRight;

    // 发送信号
    _transmitter->send(sig.code, sig.bits, sig.freq, sig.protocol, sig.pulseLength);
}

void SignalTxPage::sendEditedSignal() {
    if (_signalCount == 0 || _selectedIndex >= _signalCount) {
        return;
    }

    SignalStorage::StoredSignal& sig = _signals[_selectedIndex];

    ESP_LOGI(TAG, "发送编辑后信号: 编码:%lu 频率:%dMHz 协议:%d 位数:%d 脉宽:%dus",
             _editCode, sig.freq, sig.protocol, sig.bits, sig.pulseLength);

    // 发送编辑后的信号
    _transmitter->send(_editCode, sig.bits, sig.freq, sig.protocol, sig.pulseLength);
}

void SignalTxPage::deleteSelectedSignal() {
    if (_signalCount == 0 || _selectedIndex >= _signalCount) {
        return;
    }

    ESP_LOGI(TAG, "删除信号: %s", _signals[_selectedIndex].name);

    if (_storage->deleteSignal(_selectedIndex)) {
        // 重新加载信号列表
        loadSignals();

        // 调整选中索引
        if (_selectedIndex >= _signalCount && _signalCount > 0) {
            _selectedIndex = _signalCount - 1;
        }

        // 调整滚动偏移
        if (_scrollOffset > 0 && _scrollOffset >= _signalCount) {
            _scrollOffset = max(0, _signalCount - ITEMS_PER_PAGE);
        }

        // 退出编辑模式
        _editMode = false;
    }
}

int SignalTxPage::calcDigitCount(unsigned long code) {
    if (code == 0) return 1;
    int count = 0;
    unsigned long temp = code;
    while (temp > 0) {
        temp /= 10;
        count++;
    }
    return count;
}

int SignalTxPage::getDigitAt(int pos) {
    // pos: 0=最高位，使用固定位数格式
    char codeStr[16];
    snprintf(codeStr, sizeof(codeStr), "%0*lu", _digitCount, _editCode);
    if (pos >= 0 && pos < _digitCount) {
        return codeStr[pos] - '0';
    }
    return 0;
}

void SignalTxPage::setDigitAt(int pos, int digit) {
    // pos: 0=最高位, digit: 0-9，使用固定位数格式
    char codeStr[16];
    snprintf(codeStr, sizeof(codeStr), "%0*lu", _digitCount, _editCode);

    if (pos >= 0 && pos < _digitCount && digit >= 0 && digit <= 9) {
        codeStr[pos] = '0' + digit;
        _editCode = strtoul(codeStr, NULL, 10);
        // 不再重新计算位数，保持固定位数
    }
}

bool SignalTxPage::handleButton(ButtonEvent event) {
    if (_editMode) {
        // 编辑模式
        int maxPos = _digitCount;  // 最大位置: 数字位 + 删除 (去掉了发射按钮)

        if (_editingDigit) {
            // 正在编辑某一位数字
            switch (event) {
                case BTN_UP_LONG:
                    // 长按上键: 退出编辑该位，返回选择模式
                    ESP_LOGD(TAG, "按键: 上键长按 - 退出位编辑");
                    _editingDigit = false;
                    return true;

                case BTN_UP_SHORT:
                    // 上键: 当前位+1
                    {
                        int digit = getDigitAt(_cursorPos);
                        digit = (digit + 1) % 10;
                        setDigitAt(_cursorPos, digit);
                        ESP_LOGD(TAG, "按键: 上 - 编码变为 %lu", _editCode);
                    }
                    return true;

                case BTN_DOWN_SHORT:
                    // 下键: 当前位-1
                    {
                        int digit = getDigitAt(_cursorPos);
                        digit = (digit + 9) % 10;  // +9 等于 -1 mod 10
                        setDigitAt(_cursorPos, digit);
                        ESP_LOGD(TAG, "按键: 下 - 编码变为 %lu", _editCode);
                    }
                    return true;

                case BTN_OK_SHORT:
                    // OK键: 退出位编辑，返回选择模式
                    ESP_LOGD(TAG, "按键: OK - 确认编辑，退出位编辑");
                    _editingDigit = false;
                    return true;

                case BTN_OK_LONG:
                    // 长按OK: 发射信号
                    ESP_LOGD(TAG, "按键: OK键长按 - 发射信号");
                    sendEditedSignal();
                    return true;

                default:
                    return true;
            }
        } else {
            // 选择模式: 上下移动光标
            switch (event) {
                case BTN_UP_LONG:
                    // 长按上键: 退出编辑模式，返回列表
                    ESP_LOGD(TAG, "按键: 上键长按 - 退出编辑模式");
                    exitEditMode();
                    return true;

                case BTN_UP_SHORT:
                    // 上键: 光标左移
                    if (_cursorPos > 0) {
                        _cursorPos--;
                        ESP_LOGD(TAG, "按键: 上 - 光标移到位置 %d", _cursorPos);
                    }
                    return true;

                case BTN_DOWN_SHORT:
                    // 下键: 光标右移
                    if (_cursorPos < maxPos) {
                        _cursorPos++;
                        ESP_LOGD(TAG, "按键: 下 - 光标移到位置 %d", _cursorPos);
                    }
                    return true;

                case BTN_OK_SHORT:
                    // OK键: 根据光标位置执行操作
                    if (_cursorPos < _digitCount) {
                        // 在数字位上: 进入编辑该位
                        ESP_LOGD(TAG, "按键: OK - 进入编辑第 %d 位", _cursorPos);
                        _editingDigit = true;
                    }
                    // 删除按钮需要长按，短按不响应
                    return true;

                case BTN_OK_LONG:
                    // 长按OK: 根据光标位置
                    if (_cursorPos < _digitCount) {
                        // 在数字位上: 发射信号
                        ESP_LOGD(TAG, "按键: OK键长按 - 发射信号");
                        sendEditedSignal();
                    } else if (_cursorPos == _digitCount) {
                        // 在删除按钮上: 执行删除
                        ESP_LOGD(TAG, "按键: OK键长按 - 删除信号");
                        deleteSelectedSignal();
                    }
                    return true;

                default:
                    return true;
            }
        }
    } else {
        // 列表模式
        switch (event) {
            case BTN_UP_LONG:
                ESP_LOGD(TAG, "按键: 上键长按 - 返回主菜单");
                return false;

            case BTN_UP_SHORT:
                ESP_LOGD(TAG, "按键: 上");
                if (_signalCount > 0 && _selectedIndex > 0) {
                    _selectedIndex--;
                    if (_selectedIndex < _scrollOffset) {
                        _scrollOffset = _selectedIndex;
                    }
                }
                return true;

            case BTN_DOWN_SHORT:
                ESP_LOGD(TAG, "按键: 下");
                if (_signalCount > 0 && _selectedIndex < _signalCount - 1) {
                    _selectedIndex++;
                    if (_selectedIndex >= _scrollOffset + ITEMS_PER_PAGE) {
                        _scrollOffset = _selectedIndex - ITEMS_PER_PAGE + 1;
                    }
                }
                return true;

            case BTN_OK_SHORT:
                ESP_LOGD(TAG, "按键: 确认 - 发送信号");
                sendSelectedSignal();
                return true;

            case BTN_OK_LONG:
                // 长按OK: 进入编辑模式
                ESP_LOGD(TAG, "按键: OK键长按 - 进入编辑模式");
                enterEditMode();
                return true;

            default:
                return true;
        }
    }
}
