#ifndef SIGNAL_TX_PAGE_H
#define SIGNAL_TX_PAGE_H

#include "Page.h"
#include "SignalStorage.h"
#include "RFTransmitter.h"

/**
 * 发送模式页面
 * 显示已保存的RF信号列表，按OK键直接发送
 * 长按OK进入编辑模式，可删除信号或修改编码
 */
class SignalTxPage : public Page {
public:
    SignalTxPage(U8G2* u8g2, SignalStorage* storage, RFTransmitter* transmitter);

    void enter() override;
    void exit() override;
    void draw() override;
    bool handleButton(ButtonEvent event) override;
    bool update() override;
    const char* getTitle() override;

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

    // 编辑模式
    bool _editMode;             // 是否在编辑模式
    bool _editingDigit;         // 是否正在编辑某一位 (false=选择模式, true=编辑模式)
    int _cursorPos;             // 光标位置: 0~digitCount-1=数字位, digitCount=发射, digitCount+1=删除
    unsigned long _editCode;    // 编辑中的编码值
    int _digitCount;            // 编码的位数

    // 每页显示的信号数量
    static const int ITEMS_PER_PAGE = 3;

    void loadSignals();
    void drawSignalList();
    void drawEmptyMessage();
    void drawEditMode();
    void sendSelectedSignal();
    void sendEditedSignal();
    void deleteSelectedSignal();

    // 编辑模式辅助函数
    void enterEditMode();
    void exitEditMode();
    int calcDigitCount(unsigned long code);
    int getDigitAt(int pos);
    void setDigitAt(int pos, int digit);
};

#endif // SIGNAL_TX_PAGE_H
