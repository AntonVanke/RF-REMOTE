#ifndef SIGNAL_TX_PAGE_H
#define SIGNAL_TX_PAGE_H

#include "Page.h"
#include "SignalStorage.h"
#include "RFTransmitter.h"

/**
 * 发送模式页面
 * 显示已保存的RF信号列表，按OK键直接发送
 */
class SignalTxPage : public Page {
public:
    SignalTxPage(U8G2* u8g2, SignalStorage* storage, RFTransmitter* transmitter);

    void enter() override;
    void exit() override;
    void draw() override;
    bool handleButton(ButtonEvent event) override;
    bool update() override;
    const char* getTitle() override { return "发送模式"; }

private:
    U8G2* _u8g2;
    SignalStorage* _storage;
    RFTransmitter* _transmitter;

    // 信号列表
    static const int MAX_DISPLAY_SIGNALS = 50;
    SignalStorage::StoredSignal _signals[MAX_DISPLAY_SIGNALS];
    int _signalCount;
    int _selectedIndex;
    int _scrollOffset;

    // 发送指示器 (> 和 < 交替)
    bool _arrowRight;   // true=显示>, false=显示<

    // 每页显示的信号数量
    static const int ITEMS_PER_PAGE = 3;

    void loadSignals();
    void drawSignalList();
    void drawEmptyMessage();
    void sendSelectedSignal();
};

#endif // SIGNAL_TX_PAGE_H
