#ifndef SIGNAL_TX_PAGE_H
#define SIGNAL_TX_PAGE_H

#include "Page.h"

/**
 * 发送模式页面
 * 用于发送已保存的RF信号
 */
class SignalTxPage : public Page {
public:
    SignalTxPage(U8G2* u8g2);

    void enter() override;
    void draw() override;
    bool handleButton(ButtonEvent event) override;
    const char* getTitle() override { return "发送模式"; }

private:
    U8G2* _u8g2;

    // TODO: 添加已保存的信号列表
    // int _selectedSignal;
    // int _signalCount;
};

#endif // SIGNAL_TX_PAGE_H
