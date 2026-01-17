/**
 * @file ButtonManager.cpp
 * @brief 基于中断的按键管理模块实现
 */

#include "ButtonManager.h"

// 静态成员初始化
ButtonManager::ButtonState ButtonManager::_upBtn = {BTN_UP_PIN, false, 0, false, BTN_UP_SHORT, BTN_UP_LONG};
ButtonManager::ButtonState ButtonManager::_okBtn = {BTN_OK_PIN, false, 0, false, BTN_OK_SHORT, BTN_OK_LONG};
ButtonManager::ButtonState ButtonManager::_downBtn = {BTN_DOWN_PIN, false, 0, false, BTN_DOWN_SHORT, BTN_DOWN_LONG};

volatile ButtonEvent ButtonManager::_eventQueue[ButtonManager::EVENT_QUEUE_SIZE];
volatile int ButtonManager::_queueHead = 0;
volatile int ButtonManager::_queueTail = 0;

TaskHandle_t ButtonManager::_longPressTaskHandle = NULL;

ButtonManager::ButtonManager() {
    // 构造函数
}

void ButtonManager::begin() {
    Serial.println("初始化按键管理器...");

    // 配置按键引脚 (Active Low, 内部上拉)
    pinMode(_upBtn.pin, INPUT_PULLUP);
    pinMode(_okBtn.pin, INPUT_PULLUP);
    pinMode(_downBtn.pin, INPUT_PULLUP);

    // 附加中断 (下降沿触发 - 按键按下时)
    attachInterrupt(digitalPinToInterrupt(_upBtn.pin), handleUpButton, FALLING);
    attachInterrupt(digitalPinToInterrupt(_okBtn.pin), handleOkButton, FALLING);
    attachInterrupt(digitalPinToInterrupt(_downBtn.pin), handleDownButton, FALLING);

    // 创建长按检测任务 (优先级10, 运行在核心1)
    xTaskCreatePinnedToCore(
        checkLongPress,           // 任务函数
        "LongPressCheck",         // 任务名称
        2048,                     // 堆栈大小
        NULL,                     // 参数
        10,                       // 优先级
        &_longPressTaskHandle,    // 任务句柄
        1                         // 核心1
    );

    Serial.println("按键管理器初始化完成");
}

void IRAM_ATTR ButtonManager::handleUpButton() {
    unsigned long now = millis();

    // 防抖检测
    if (_upBtn.pressed || (now - _upBtn.pressTime < DEBOUNCE_DELAY)) {
        return;
    }

    _upBtn.pressed = true;
    _upBtn.pressTime = now;
    _upBtn.longPressTriggered = false;
}

void IRAM_ATTR ButtonManager::handleOkButton() {
    unsigned long now = millis();

    if (_okBtn.pressed || (now - _okBtn.pressTime < DEBOUNCE_DELAY)) {
        return;
    }

    _okBtn.pressed = true;
    _okBtn.pressTime = now;
    _okBtn.longPressTriggered = false;
}

void IRAM_ATTR ButtonManager::handleDownButton() {
    unsigned long now = millis();

    if (_downBtn.pressed || (now - _downBtn.pressTime < DEBOUNCE_DELAY)) {
        return;
    }

    _downBtn.pressed = true;
    _downBtn.pressTime = now;
    _downBtn.longPressTriggered = false;
}

void ButtonManager::checkLongPress(void* parameter) {
    while (true) {
        unsigned long now = millis();

        // 检查上键
        if (_upBtn.pressed) {
            // 立即检查按键释放状态
            bool isReleased = digitalRead(_upBtn.pin) == HIGH;

            if (!_upBtn.longPressTriggered && (now - _upBtn.pressTime >= LONG_PRESS_TIME)) {
                // 长按触发
                _upBtn.longPressTriggered = true;
                pushEvent(_upBtn.longEvent);
            }

            // 检测按键释放
            if (isReleased) {
                // 延迟5ms再次确认（防抖）
                vTaskDelay(pdMS_TO_TICKS(5));
                if (digitalRead(_upBtn.pin) == HIGH) {
                    if (!_upBtn.longPressTriggered) {
                        // 短按触发
                        pushEvent(_upBtn.shortEvent);
                    }
                    _upBtn.pressed = false;
                }
            }
        }

        // 检查确认键
        if (_okBtn.pressed) {
            bool isReleased = digitalRead(_okBtn.pin) == HIGH;

            if (!_okBtn.longPressTriggered && (now - _okBtn.pressTime >= LONG_PRESS_TIME)) {
                _okBtn.longPressTriggered = true;
                pushEvent(_okBtn.longEvent);
            }

            if (isReleased) {
                vTaskDelay(pdMS_TO_TICKS(5));
                if (digitalRead(_okBtn.pin) == HIGH) {
                    if (!_okBtn.longPressTriggered) {
                        pushEvent(_okBtn.shortEvent);
                    }
                    _okBtn.pressed = false;
                }
            }
        }

        // 检查下键
        if (_downBtn.pressed) {
            bool isReleased = digitalRead(_downBtn.pin) == HIGH;

            if (!_downBtn.longPressTriggered && (now - _downBtn.pressTime >= LONG_PRESS_TIME)) {
                _downBtn.longPressTriggered = true;
                pushEvent(_downBtn.longEvent);
            }

            if (isReleased) {
                vTaskDelay(pdMS_TO_TICKS(5));
                if (digitalRead(_downBtn.pin) == HIGH) {
                    if (!_downBtn.longPressTriggered) {
                        pushEvent(_downBtn.shortEvent);
                    }
                    _downBtn.pressed = false;
                }
            }
        }

        // 缩短检查间隔到5ms，提高响应速度
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void ButtonManager::pushEvent(ButtonEvent event) {
    int nextTail = (_queueTail + 1) % EVENT_QUEUE_SIZE;

    // 如果队列未满，添加事件
    if (nextTail != _queueHead) {
        _eventQueue[_queueTail] = event;
        _queueTail = nextTail;
    }
}

ButtonEvent ButtonManager::getEvent() {
    if (_queueHead == _queueTail) {
        return BTN_NONE;
    }

    ButtonEvent event = _eventQueue[_queueHead];
    _queueHead = (_queueHead + 1) % EVENT_QUEUE_SIZE;

    return event;
}

bool ButtonManager::hasEvent() {
    return _queueHead != _queueTail;
}
