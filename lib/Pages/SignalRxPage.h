#ifndef SIGNAL_RX_PAGE_H
#define SIGNAL_RX_PAGE_H

#include "Page.h"
#include "RFReceiver.h"
#include "SignalStorage.h"

/**
 * 信号接收页面
 * 用于接收和显示RF信号，自动保存到Flash
 */
class SignalRxPage : public Page {
public:
    SignalRxPage(U8G2* u8g2, RFReceiver* receiver, SignalStorage* storage);

    void enter() override;
    void exit();  // 退出页面时调用
    void draw() override;
    bool handleButton(ButtonEvent event) override;
    const char* getTitle() override { return "信号接收"; }
    bool needsConstantRefresh() override { return _debugMode; }  // 调试模式时实时刷新
    bool update() override;

private:
    U8G2* _u8g2;
    RFReceiver* _receiver;
    SignalStorage* _storage;

    // 是否已收到过信号
    bool _hasSignal;

    // 当前显示的信号
    RFReceiver::Signal _currentSignal;
    char _savedName[32];
    bool _signalExists;  // 信号是否已存在（未保存）

    // 防重复机制
    unsigned long _lastCode;      // 上次接收的编码
    unsigned long _lastCodeTime;  // 上次接收时间
    static const unsigned long DUPLICATE_WINDOW = 2000;  // 2秒防重复窗口

    // 调试模式
    bool _debugMode;              // 是否显示调试信息
    unsigned long _lastDebugUpdate; // 上次调试信息更新时间

    // 辅助函数
    void drawWaiting();
    void drawSignalInfo();
    void drawDebugInfo();         // 绘制调试信息
};

#endif // SIGNAL_RX_PAGE_H
