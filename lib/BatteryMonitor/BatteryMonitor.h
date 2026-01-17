/**
 * @file BatteryMonitor.h
 * @brief ESP32-C3电池电压监测库
 *
 * 支持通过ADC和分压电路测量电池电压
 * 包含两点线性校准功能以提高测量精度
 */

#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>
#include "pin_config.h"

class BatteryMonitor {
public:
    /**
     * @brief 构造函数 (使用默认配置)
     * GPIO4, R25=100kΩ, R24=10kΩ
     * 已预设两点校准参数
     */
    BatteryMonitor();

    /**
     * @brief 初始化ADC (自动配置)
     */
    void begin();

    /**
     * @brief 读取电池电压
     * @return 校准后的电池电压 (V)
     */
    float readVoltage();

    /**
     * @brief 读取ADC原始值
     * @return ADC原始读数
     */
    int readRawADC();

    /**
     * @brief 设置两点线性校准参数
     * @param slope 校准斜率
     * @param offset 校准偏移 (V)
     */
    void setCalibration(float slope, float offset);

    /**
     * @brief 自动计算两点校准参数
     * @param measured1 第一个测量值 (V)
     * @param actual1 第一个实际值 (V)
     * @param measured2 第二个测量值 (V)
     * @param actual2 第二个实际值 (V)
     */
    void calibrate(float measured1, float actual1, float measured2, float actual2);

    /**
     * @brief 设置ADC参考电压
     * @param vref 参考电压 (V)
     */
    void setVref(float vref);

    /**
     * @brief 获取ADC引脚电压
     * @return ADC引脚电压 (V)
     */
    float getADCVoltage();

private:
    uint8_t _adcPin;              // ADC引脚
    float _voltageDividerRatio;   // 分压比
    float _adcResolution;         // ADC分辨率
    float _adcVref;               // ADC参考电压
    float _calibrationSlope;      // 校准斜率
    float _calibrationOffset;     // 校准偏移
};

#endif // BATTERY_MONITOR_H
