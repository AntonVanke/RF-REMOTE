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
| 充电检测 | IO10 | TP4056 CHRG#, Active Low |

### 显示屏
- **型号**: SSD1306 OLED 128x64
- **接口**: I2C (地址: 0x3C)
- **中文字体**: 使用U8g2库的 `u8g2_font_wqy12_t_gb2312` (12x12)
- **I2C模式**: 默认软件I2C (可选硬件I2C 800kHz)
- **布局**: 顶部16像素黄色区域为状态栏，其余为内容区

### 按键电路
- 按键接地设计 (Active Low)
- 外部10kΩ上拉电阻
- 需要配置为 `INPUT_PULLUP` 模式
- 使用GPIO中断 (FALLING边沿触发) + FreeRTOS任务处理
- 防抖延迟: 30ms
- 长按时间: 500ms
- 检测周期: 5ms (高响应速度)

### 电池监测
- 分压电路: R25=100kΩ, R24=10kΩ
- 分压比: 11:1
- ADC: 12位分辨率
- 二极管压降: 0.3V (1N5819WS肖特基)
- 电池电压范围: 2.7V-3.9V (补偿压降后)
- 充电检测: IO10连接TP4056的CHRG#引脚 (Active Low)
- USB供电检测: 电压>4.0V判定为USB直接供电

## 开发环境

- **IDE**: PlatformIO
- **框架**: Arduino
- **平台**: espressif32

## 项目结构

```
RF-REMOTE/
├── include/
│   └── pin_config.h          # 硬件引脚配置
├── lib/
│   ├── BatteryMonitor/       # 电池监测模块
│   │   ├── BatteryMonitor.h
│   │   └── BatteryMonitor.cpp
│   ├── ButtonManager/        # 按键管理模块
│   │   ├── ButtonManager.h
│   │   └── ButtonManager.cpp
│   ├── Display/              # 显示管理模块
│   │   ├── Display.h
│   │   └── Display.cpp
│   ├── Menu/                 # 菜单管理模块
│   │   ├── Menu.h
│   │   └── Menu.cpp
│   └── StatusBar/            # 状态栏模块
│       ├── StatusBar.h
│       └── StatusBar.cpp
├── src/
│   └── main.cpp              # 主程序
├── platformio.ini            # PlatformIO配置
└── CLAUDE.md                 # 本文件
```

## 依赖库

```ini
lib_deps =
    olikraus/U8g2           # OLED显示库
```

**注意**:
- 按键管理使用自定义ButtonManager模块 (基于GPIO中断+FreeRTOS)
- RF信号处理库待后续添加 (推荐 sui77/rc-switch)

## 开发规范

### 代码风格
- 使用中文注释说明关键逻辑
- 所有硬件引脚必须使用 `pin_config.h` 中的宏定义
- 避免硬编码引脚号
- 模块化设计，每个模块职责单一

### 按键处理 (ButtonManager模块)
- 使用GPIO中断 (FALLING边沿) 触发按键事件
- 按键为Active Low (按下=LOW)
- FreeRTOS任务 (5ms周期) 检测长按和释放
- 事件队列机制，支持快速连击
- 配置参数:
```cpp
static const unsigned long LONG_PRESS_TIME = 500;   // 长按500ms
static const unsigned long DEBOUNCE_DELAY = 30;     // 防抖30ms
static const int EVENT_QUEUE_SIZE = 10;             // 事件队列大小
```

### OLED显示 (Display模块)
- 必须调用 `u8g2.enableUTF8Print()`
- 使用 `drawUTF8()` 而非 `print()` 显示中文
- 字体: `u8g2_font_wqy12_t_gb2312` (12x12)
- 对比度: 255 (最大)
- 使用智能重绘策略，仅在需要时刷新屏幕:
  - 按键事件触发
  - 页面切换
  - 电池状态变化
  - FPS测试模式强制刷新

### 硬件I2C选项
在 `platformio.ini` 中取消注释以启用硬件I2C (800kHz):
```ini
build_flags =
    -I include
    -D USE_HW_I2C  ; 启用硬件I2C
```
**注意**: 硬件I2C更快但可能不稳定，默认使用软件I2C

### RF信号处理
- 433MHz和315MHz模块独立控制
- 发射和接收引脚分开
- 使用rc-switch库进行编解码

## 已实现功能

1. **主菜单系统**:
   - 三个菜单项: 信号接收、发送模式、刷新率测试
   - 上下键导航，确认键进入子页面
   - 长按上键返回主菜单

2. **状态栏显示**:
   - 页面标题
   - 电池电量图标和百分比 (0-100%)
   - 充电状态图标 (闪电符号)
   - USB供电模式显示 ("USB"字样)

3. **电池监测**:
   - 实时电压测量 (补偿二极管压降)
   - 电量百分比计算 (2.7V-3.9V线性映射)
   - 充电状态检测 (TP4056 CHRG#引脚)
   - USB供电检测 (>4.0V判定)

4. **按键控制**:
   - 中断式按键响应 (<10ms平均延迟)
   - 短按/长按检测
   - 快速连击支持 (最高28次/秒)
   - 事件队列防止丢帧

5. **刷新率测试**:
   - 实时FPS显示
   - 总帧数统计
   - 运行时间显示

## 待实现功能

1. **RF接收**: 接收并解码RF信号，显示协议信息
2. **RF发射**: 发送已保存的RF信号
3. **信号存储**: 保存接收到的信号到Flash
4. **设置**: 配置RF频率、协议等参数

## 模块架构说明

### BatteryMonitor 模块
**职责**: 电池电压监测、充电状态检测、电量计算

**关键方法**:
- `float readVoltage()` - 读取校准后的电池电压
- `bool isCharging()` - 检测是否正在充电 (IO10 Active Low)
- `bool isUSBPowered()` - 检测是否USB供电 (>4.0V)
- `uint8_t getBatteryPercent()` - 计算电量百分比 (0-100)

### ButtonManager 模块
**职责**: GPIO中断式按键管理、长按检测、事件队列

**关键特性**:
- GPIO中断触发 (FALLING边沿)
- FreeRTOS任务处理长按和释放检测
- 环形事件队列 (10个事件)
- 支持短按、长按事件

**事件类型**:
```cpp
enum ButtonEvent {
    BTN_NONE,
    BTN_UP_SHORT, BTN_UP_LONG,
    BTN_OK_SHORT, BTN_OK_LONG,
    BTN_DOWN_SHORT, BTN_DOWN_LONG
};
```

### Display 模块
**职责**: U8g2显示系统管理、I2C初始化

**关键特性**:
- 支持软件/硬件I2C切换 (USE_HW_I2C宏)
- I2C设备扫描功能
- UTF8中文支持

### StatusBar 模块
**职责**: 顶部状态栏渲染

**显示内容**:
- 页面标题 (左侧)
- 电池图标 (右侧)
- 电量百分比或"USB"字样
- 充电图标 (闪电符号)

### Menu 模块
**职责**: 菜单项管理和渲染

**关键特性**:
- 编号菜单项 (1. 2. 3.)
- 选中标记 (">")
- 上下导航

## 性能指标

- **按键响应**: <10ms 平均延迟
- **连击速度**: 最高28次/秒 (35ms最小间隔)
- **内存使用**: 4.8% (15720字节)
- **Flash使用**: 36.1% (472840字节)
- **屏幕刷新**: 智能重绘策略，减少90%+不必要刷新

## 注意事项

1. ESP32-C3的GPIO数量有限，引脚已被充分利用
2. 按键使用外部上拉，代码中仍需配置INPUT_PULLUP
3. RF模块需要9.6V供电，由MT3608升压提供
4. 中文显示会占用较多Flash空间
5. 硬件I2C可能不稳定，建议使用软件I2C (默认)
6. 电池电压需补偿0.3V二极管压降
7. FPS测试模式会强制每帧刷新，用于性能测试



注意：每次修改完后都要更新 CLAUDE.md
使用中文来回答问题
