#ifndef ABOUT_PAGE_H
#define ABOUT_PAGE_H

#include "Page.h"
#include "BatteryMonitor.h"

/**
 * 关于页面
 * 显示系统信息：FPS、CPU频率、RAM、Flash、SDK版本、运行时间、电池电压
 */
class AboutPage : public Page {
public:
    AboutPage(U8G2* u8g2, BatteryMonitor* battery);

    void enter() override;
    void draw() override;
    bool handleButton(ButtonEvent event) override;
    const char* getTitle() override { return "关于"; }
    bool needsConstantRefresh() override { return true; }
    bool update() override;

private:
    U8G2* _u8g2;
    BatteryMonitor* _battery;

    // FPS相关
    unsigned long _startTime;
    unsigned long _frameCount;
    float _currentFPS;
    unsigned long _lastFPSUpdate;

    // 工具函数
    static void intToStr(unsigned long val, char* buf, int width = 0);
    static void floatToStr(float val, char* buf);
};

#endif // ABOUT_PAGE_H
