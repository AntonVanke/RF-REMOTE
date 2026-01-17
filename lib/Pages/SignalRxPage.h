#ifndef SIGNAL_RX_PAGE_H
#define SIGNAL_RX_PAGE_H

#include "Page.h"

/**
 * 信号接收页面
 * 用于接收和显示RF信号
 */
class SignalRxPage : public Page {
public:
    SignalRxPage(U8G2* u8g2);

    void enter() override;
    void draw() override;
    bool handleButton(ButtonEvent event) override;
    const char* getTitle() override { return "信号接收"; }

private:
    U8G2* _u8g2;

    // TODO: 添加接收到的信号数据
    // unsigned long _receivedCode;
    // int _protocol;
    // int _bitLength;
};

#endif // SIGNAL_RX_PAGE_H
