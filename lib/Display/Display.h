/**
 * @file Display.h
 * @brief OLED显示管理模块
 *
 * 统一管理U8g2显示系统，提供I2C扫描和初始化功能
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <U8g2lib.h>
#include <Wire.h>
#include "pin_config.h"

class StatusBar;
class Menu;

class Display {
public:
    /**
     * @brief 构造函数
     */
    Display();

    /**
     * @brief 初始化OLED显示屏
     * @return true=成功, false=失败
     */
    bool begin();

    /**
     * @brief 扫描I2C总线上的设备
     */
    void scanI2C();

    /**
     * @brief 获取U8g2实例指针
     * @return U8g2实例指针
     */
    U8G2* getU8g2();

private:
    U8G2_SSD1306_128X64_NONAME_F_SW_I2C _u8g2;
};

#endif // DISPLAY_H
