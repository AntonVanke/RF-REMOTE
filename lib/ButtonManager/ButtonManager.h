/**
 * @file ButtonManager.h
 * @brief 基于中断的按键管理模块
 *
 * 支持三个按键（上、确认、下）的中断处理
 * 支持短按和长按检测（长按500ms）
 * 按键为Active Low (按下时下拉到GND)
 */

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include "pin_config.h"

// 按键事件类型
enum ButtonEvent {
    BTN_NONE = 0,
    BTN_UP_SHORT,       // 上键短按
    BTN_UP_LONG,        // 上键长按 (返回)
    BTN_OK_SHORT,       // 确认键短按
    BTN_OK_LONG,        // 确认键长按
    BTN_DOWN_SHORT,     // 下键短按
    BTN_DOWN_LONG       // 下键长按
};

class ButtonManager {
public:
    /**
     * @brief 构造函数
     */
    ButtonManager();

    /**
     * @brief 初始化按键和中断
     */
    void begin();

    /**
     * @brief 获取按键事件 (非阻塞)
     * @return 按键事件，如果没有事件返回BTN_NONE
     */
    ButtonEvent getEvent();

    /**
     * @brief 检查是否有待处理的事件
     * @return true=有事件, false=无事件
     */
    bool hasEvent();

private:
    // 静态中断处理函数
    static void IRAM_ATTR handleUpButton();
    static void IRAM_ATTR handleOkButton();
    static void IRAM_ATTR handleDownButton();

    // 长按检测任务
    static void checkLongPress(void* parameter);

    // 按键状态结构
    struct ButtonState {
        uint8_t pin;
        volatile bool pressed;
        volatile unsigned long pressTime;
        volatile bool longPressTriggered;
        ButtonEvent shortEvent;
        ButtonEvent longEvent;
    };

    static ButtonState _upBtn;
    static ButtonState _okBtn;
    static ButtonState _downBtn;

    // 事件队列 (简单的环形缓冲区)
    static const int EVENT_QUEUE_SIZE = 10;
    static volatile ButtonEvent _eventQueue[EVENT_QUEUE_SIZE];
    static volatile int _queueHead;
    static volatile int _queueTail;

    // 添加事件到队列
    static void pushEvent(ButtonEvent event);

    // 长按检测时间 (ms)
    static const unsigned long LONG_PRESS_TIME = 500;

    // 防抖延迟 (ms) - 优化为30ms提高响应速度
    static const unsigned long DEBOUNCE_DELAY = 30;

    // 长按检测任务句柄
    static TaskHandle_t _longPressTaskHandle;
};

#endif // BUTTON_MANAGER_H
