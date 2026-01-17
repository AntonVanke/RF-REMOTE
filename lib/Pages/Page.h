#ifndef PAGE_H
#define PAGE_H

#include <U8g2lib.h>
#include "ButtonManager.h"

/**
 * 页面基类
 * 所有页面都需要继承此类并实现虚函数
 */
class Page {
public:
    virtual ~Page() {}

    /**
     * 进入页面时调用，用于初始化页面状态
     */
    virtual void enter() = 0;

    /**
     * 退出页面时调用，用于清理资源
     */
    virtual void exit() {}

    /**
     * 绘制页面内容 (不包括状态栏)
     */
    virtual void draw() = 0;

    /**
     * 处理按键事件
     * @param event 按键事件
     * @return true表示事件已处理，false表示未处理（可能需要返回上级）
     */
    virtual bool handleButton(ButtonEvent event) = 0;

    /**
     * 获取页面标题 (用于状态栏显示)
     */
    virtual const char* getTitle() = 0;

    /**
     * 是否需要强制每帧刷新
     * 默认false，关于页面等需要计算FPS的页面返回true
     */
    virtual bool needsConstantRefresh() { return false; }

    /**
     * 页面更新 (每帧调用，用于更新内部状态)
     * @return true表示需要重绘
     */
    virtual bool update() { return false; }
};

#endif // PAGE_H
