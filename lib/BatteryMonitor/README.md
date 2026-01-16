# BatteryMonitor Library

ESP32-C3电池电压监测库，支持ADC分压电路测量和两点线性校准。

## 功能特性

- ✅ 支持自定义分压电路配置
- ✅ 两点线性校准提高精度
- ✅ 灵活的ADC参考电压设置
- ✅ 简单易用的API接口
- ✅ 支持12位ADC分辨率

## 硬件连接

```
VCC ----+---- R25(100kΩ) ----+---- BAT_ADC (GPIO4)
        |                     |
        |                     +---- R24(10kΩ) ----+---- GND
        |                     |                    |
        |                     +---- C20(22uF) ----+
```

## 基本使用

```cpp
#include <BatteryMonitor.h>

// 创建监测对象
BatteryMonitor battery(4, 100.0, 10.0);  // GPIO4, R1=100kΩ, R2=10kΩ

void setup() {
  Serial.begin(115200);

  // 初始化
  battery.begin(12);  // 12位ADC

  // 两点校准
  battery.calibrate(5.8, 5.16, 3.64, 3.94);
}

void loop() {
  float voltage = battery.readVoltage();
  Serial.println(voltage);
  delay(1000);
}
```

## API参考

### 构造函数

```cpp
BatteryMonitor(uint8_t adcPin, float r1 = 100.0, float r2 = 10.0)
```
- `adcPin`: ADC引脚编号
- `r1`: 上拉电阻 (kΩ)
- `r2`: 下拉电阻 (kΩ)

### 方法

#### `void begin(uint8_t resolution = 12)`
初始化ADC，设置分辨率（默认12位）

#### `float readVoltage()`
读取校准后的电池电压 (V)

#### `int readRawADC()`
读取ADC原始值（0-4095）

#### `void calibrate(float measured1, float actual1, float measured2, float actual2)`
两点校准法设置校准参数
- `measured1, measured2`: 测量值 (V)
- `actual1, actual2`: 实际值 (V)

#### `void setCalibration(float slope, float offset)`
直接设置校准参数
- `slope`: 校准斜率
- `offset`: 校准偏移 (V)

#### `void setVref(float vref)`
设置ADC参考电压 (V)

#### `float getADCVoltage()`
获取ADC引脚电压 (V)

## 校准方法

### 两点校准（推荐）

1. 使用万用表测量实际电压
2. 记录测量值和实际值
3. 调用 `calibrate()` 方法

```cpp
// 示例：测量5.8V实际5.16V，测量3.64V实际3.94V
battery.calibrate(5.8, 5.16, 3.64, 3.94);
```

### 手动校准

直接设置计算好的参数：

```cpp
battery.setCalibration(0.565, 1.88);
```

## 许可证

MIT License
