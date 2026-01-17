/**
 * @file BatteryMonitor.cpp
 * @brief ESP32-C3电池电压监测库实现
 */

#include "BatteryMonitor.h"

BatteryMonitor::BatteryMonitor()
    : _adcPin(BAT_ADC_PIN),
      _chargePin(CHARGE_DETECT_PIN),
      _adcResolution(BAT_ADC_RESOLUTION),
      _adcVref(BAT_ADC_VREF),
      _batteryMinVoltage(BAT_VOLTAGE_MIN),
      _batteryMaxVoltage(BAT_VOLTAGE_MAX),
      _calibrationSlope(0.565),
      _calibrationOffset(1.88) {
    // 计算分压比: (R1 + R2) / R2
    _voltageDividerRatio = (float)(BAT_DIVIDER_R1 + BAT_DIVIDER_R2) / BAT_DIVIDER_R2;
}

void BatteryMonitor::begin() {
    // 设置ADC为12位分辨率
    analogReadResolution(12);

    // 设置ADC衰减 (0-3.3V测量范围)
    analogSetAttenuation(ADC_11db);

    // 配置充电检测引脚 (Active Low, 充电时为LOW)
    pinMode(_chargePin, INPUT_PULLUP);
}

float BatteryMonitor::readVoltage() {
    // 读取ADC原始值
    int adcValue = analogRead(_adcPin);

    // 转换为ADC引脚电压
    float adcVoltage = (adcValue / _adcResolution) * _adcVref;

    // 计算测量电压（考虑分压电路）
    float measuredVoltage = adcVoltage * _voltageDividerRatio;

    // 使用两点线性校准得到实际电压
    float actualVoltage = measuredVoltage * _calibrationSlope + _calibrationOffset;

    return actualVoltage;
}

bool BatteryMonitor::isCharging() {
    // CHRG# 是 Active Low: 充电时为LOW
    return digitalRead(_chargePin) == LOW;
}

bool BatteryMonitor::isUSBPowered() {
    float voltage = readVoltage();
    // USB供电时电压通常>4.0V (5V经过二极管和分压后)
    return voltage > 4.0;
}

uint8_t BatteryMonitor::getBatteryPercent() {
    float voltage = readVoltage();

    // 限制在范围内
    if (voltage < _batteryMinVoltage) voltage = _batteryMinVoltage;
    if (voltage > _batteryMaxVoltage) voltage = _batteryMaxVoltage;

    // 线性计算百分比
    float percent = (voltage - _batteryMinVoltage) /
                    (_batteryMaxVoltage - _batteryMinVoltage) * 100.0;

    return (uint8_t)percent;
}

void BatteryMonitor::setBatteryRange(float minV, float maxV) {
    _batteryMinVoltage = minV;
    _batteryMaxVoltage = maxV;
}

int BatteryMonitor::readRawADC() {
    return analogRead(_adcPin);
}

void BatteryMonitor::setCalibration(float slope, float offset) {
    _calibrationSlope = slope;
    _calibrationOffset = offset;
}

void BatteryMonitor::calibrate(float measured1, float actual1, float measured2, float actual2) {
    // 计算线性校准参数
    // 斜率 = (y2 - y1) / (x2 - x1)
    _calibrationSlope = (actual2 - actual1) / (measured2 - measured1);

    // 偏移 = y1 - slope * x1
    _calibrationOffset = actual1 - _calibrationSlope * measured1;
}

void BatteryMonitor::setVref(float vref) {
    _adcVref = vref;
}

float BatteryMonitor::getADCVoltage() {
    int adcValue = analogRead(_adcPin);
    return (adcValue / _adcResolution) * _adcVref;
}
