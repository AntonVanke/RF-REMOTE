# RF-REMOTE

ESP32-C3 射频遥控器项目，带电池电压监测功能。

## 硬件平台

- **开发板**: Seeed XIAO ESP32-C3
- **IDE**: PlatformIO

## 功能特性

### 电池电压监测

通过ADC和分压电路实时监测电池电压。

- **ADC引脚**: GPIO4 (BAT_ADC)
- **分压电路**: R25=100kΩ, R24=10kΩ, C20=22uF
- **测量精度**: 两点线性校准
- **测量范围**: 0-36V

## 项目结构

```
RF-REMOTE/
├── lib/
│   └── BatteryMonitor/     # 电池监测库
│       ├── BatteryMonitor.h
│       ├── BatteryMonitor.cpp
│       └── README.md
├── src/
│   └── main.cpp            # 主程序
└── platformio.ini          # PlatformIO配置
```

## 使用说明

### 编译上传

```bash
pio run -t upload
```

### 串口监视

```bash
pio device monitor
```

## BatteryMonitor 库

简单易用的电池电压监测库，所有配置已预设，无需额外配置。

```cpp
#include <BatteryMonitor.h>

BatteryMonitor battery;

void setup() {
  Serial.begin(115200);
  battery.begin();
}

void loop() {
  float voltage = battery.readVoltage();
  Serial.println(voltage);
  delay(1000);
}
```

详细说明见 `lib/BatteryMonitor/README.md`

## 电路原理图

项目包含完整的电路原理图 (PDF格式)。

## 许可证

MIT License
