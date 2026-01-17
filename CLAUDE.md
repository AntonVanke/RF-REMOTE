# RF遥控器项目 - Claude开发指南

## 项目概述

这是一个基于 **Seeed XIAO ESP32-C3** 的RF遥控器项目，支持433MHz和315MHz信号的发射与接收。

## 硬件配置

### MCU
- **型号**: Seeed XIAO ESP32-C3 (RISC-V架构)
- **供电**: 3.3V (AMS1117-3.3稳压)
- **RF模块供电**: 9.6V (MT3608升压)

### 引脚分配 (详见 `include/pin_config.h`)

| 功能 | 引脚 | 说明 |
|------|------|------|
| OLED SCL | IO6 | I2C时钟线 |
| OLED SDA | IO8 | I2C数据线 |
| 上键 | IO5 | Active Low, 外部上拉 |
| 确认键 | IO9 | Active Low, 外部上拉 |
| 下键 | IO7 | Active Low, 外部上拉 |
| 433MHz TX | IO0 | 发射模块 |
| 433MHz RX | IO1 | 接收模块 |
| 315MHz TX | IO2 | 发射模块 |
| 315MHz RX | IO3 | 接收模块 |
| 电池ADC | IO4 | 分压比11:1 |

### 显示屏
- **型号**: SSD1306 OLED 128x64
- **接口**: I2C (地址: 0x3C)
- **中文字体**: 使用U8g2库的 `u8g2_font_wqy16_t_gb2312b` (16x16)

### 按键电路
- 按键接地设计 (Active Low)
- 外部10kΩ上拉电阻
- 需要配置为 `INPUT_PULLUP` 模式
- 使用中断方式处理按键以提高响应速度

### 电池监测
- 分压电路: R25=100kΩ, R24=10kΩ
- 分压比: 11:1
- ADC: 12位分辨率

## 开发环境

- **IDE**: PlatformIO
- **框架**: Arduino
- **平台**: espressif32

## 项目结构

```
RF-REMOTE/
├── include/
│   └── pin_config.h      # 硬件引脚配置
├── lib/
│   └── BatteryMonitor/   # 电池监测库
├── src/
│   └── main.cpp          # 主程序
├── platformio.ini        # PlatformIO配置
└── CLAUDE.md             # 本文件
```

## 推荐库

```ini
lib_deps =
    olikraus/U8g2           # OLED显示
    lennarthennigs/Button2  # 按键处理
    sui77/rc-switch         # RF信号处理
```

## 开发规范

### 代码风格
- 使用中文注释说明关键逻辑
- 所有硬件引脚必须使用 `pin_config.h` 中的宏定义
- 避免硬编码引脚号

### 按键处理
- 使用Button2库的中断模式
- 按键为Active Low (按下=LOW)
- 配置示例:
```cpp
Button2 btn;
btn.begin(BTN_UP_PIN, INPUT_PULLUP, false);  // false表示低电平有效
btn.setDebounceTime(50);      // 防抖50ms
btn.setLongClickTime(1000);   // 长按1000ms
```

### OLED显示中文
- 必须调用 `u8g2.enableUTF8Print()`
- 使用 `drawUTF8()` 而非 `print()` 显示中文
- 字体: `u8g2_font_wqy16_t_gb2312b`

### RF信号处理
- 433MHz和315MHz模块独立控制
- 发射和接收引脚分开
- 使用rc-switch库进行编解码

## 功能需求

1. **主菜单**: 显示功能选项列表，支持滚动
2. **RF接收**: 接收并解码RF信号，显示协议信息
3. **RF发射**: 发送已保存的RF信号
4. **信号存储**: 保存接收到的信号到Flash
5. **电池监测**: 显示当前电池电压和电量百分比
6. **设置**: 配置RF频率、协议等参数

## 注意事项

1. ESP32-C3的GPIO数量有限，引脚已被充分利用
2. 按键使用外部上拉，代码中仍需配置INPUT_PULLUP
3. RF模块需要9.6V供电，由MT3608升压提供
4. 中文显示会占用较多Flash空间
