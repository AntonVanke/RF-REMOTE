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
│   ├── Pages/                # 页面模块 (模块化页面内容)
│   │   ├── Page.h            # 页面基类
│   │   ├── AboutPage.h       # 关于页面
│   │   ├── AboutPage.cpp
│   │   ├── SignalRxPage.h    # 信号接收页面
│   │   ├── SignalRxPage.cpp
│   │   ├── SignalTxPage.h    # 发送模式页面
│   │   └── SignalTxPage.cpp
│   ├── RCSwitch315/          # 315MHz RF收发库 (本地库，基于rc-switch)
│   │   ├── RCSwitch315.h
│   │   └── RCSwitch315.cpp
│   ├── RCSwitch433/          # 433MHz RF收发库 (本地库，基于rc-switch)
│   │   ├── RCSwitch433.h
│   │   └── RCSwitch433.cpp
│   ├── RFReceiver/           # RF信号接收模块 (双频同时监听)
│   │   ├── RFReceiver.h
│   │   └── RFReceiver.cpp
│   ├── RFTransmitter/        # RF信号发送模块 (双频发送)
│   │   ├── RFTransmitter.h
│   │   └── RFTransmitter.cpp
│   ├── SignalStorage/        # 信号存储模块
│   │   ├── SignalStorage.h
│   │   └── SignalStorage.cpp
│   └── StatusBar/            # 状态栏模块
│       ├── StatusBar.h
│       └── StatusBar.cpp
├── src/
│   └── main.cpp              # 主程序 (页面路由和初始化)
├── platformio.ini            # PlatformIO配置
└── CLAUDE.md                 # 本文件
```

## 依赖库

```ini
lib_deps =
    olikraus/U8g2           # OLED显示库
    bblanchon/ArduinoJson   # JSON处理库
```

**内置库**:
- LittleFS: ESP32内置文件系统，用于信号存储

**本地库** (lib/目录):
- RCSwitch433: 433MHz RF收发库，基于rc-switch修改，支持调试功能
- RCSwitch315: 315MHz RF收发库，基于rc-switch修改，支持调试功能

**注意**:
- 按键管理使用自定义ButtonManager模块 (基于GPIO中断+FreeRTOS)
- RF收发使用本地库，不再依赖外部rc-switch库

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
- **双频同时监听**: 使用两个独立的本地库
  - RCSwitch433: 处理433MHz (RX: IO1, TX: IO0)
  - RCSwitch315: 处理315MHz (RX: IO3, TX: IO2)
- 两个库各自维护独立的静态变量，可同时工作
- 支持接收和发送功能，12种协议

### 日志系统
使用ESP-IDF内置日志宏替代Serial.print，支持日志级别过滤。

**日志级别配置** (platformio.ini):
```ini
build_flags =
    -DCORE_DEBUG_LEVEL=4  ; 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug, 5=Verbose
```

**日志宏使用**:
```cpp
#include <esp32-hal-log.h>
static const char* TAG = "ModuleName";

ESP_LOGE(TAG, "错误: %s", error);     // Error级别
ESP_LOGW(TAG, "警告: %s", warning);   // Warning级别
ESP_LOGI(TAG, "信息: %s", info);      // Info级别
ESP_LOGD(TAG, "调试: %s", debug);     // Debug级别
ESP_LOGV(TAG, "详细: %s", verbose);   // Verbose级别
```

**各模块TAG定义**:
| 模块 | TAG |
|------|-----|
| main.cpp | "Main" |
| Display.cpp | "Display" |
| ButtonManager.cpp | "Button" |
| AboutPage.cpp | "AboutPage" |
| SignalRxPage.cpp | "SignalRx" |
| SignalTxPage.cpp | "SignalTx" |
| RFReceiver.cpp | "RFReceiver" |
| RFTransmitter.cpp | "RFTransmitter" |
| SignalStorage.cpp | "Storage" |

## 已实现功能

1. **主菜单系统**:
   - 三个菜单项: 信号接收、发送模式、关于
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

5. **关于页面**:
   - 实时FPS显示
   - CPU频率、RAM剩余、Flash大小
   - SDK版本、运行时间
   - 电池电压 (补偿0.27V二极管压降)

6. **页面模块化架构**:
   - Page基类定义统一接口
   - 各页面独立模块，职责清晰
   - main.cpp仅负责路由和初始化

7. **RF信号接收**:
   - 支持433MHz和315MHz**同时监听** (中断方式)
   - 使用双库架构: RCSwitch(433MHz) + RCSwitch315(315MHz)
   - 显示接收到的编码、协议、位数、频率
   - 支持PT2262/PT2260/EV1527等常见协议 (12种)
   - 接收容差80% (默认60%)，提高兼容性
   - 信息持续显示，直到收到新信号
   - 2秒防重复窗口，避免连续信号干扰

8. **信号存储**:
   - 使用LittleFS文件系统
   - JSON格式存储 (`/signals.json`)
   - 自动保存接收到的信号
   - 重复信号检测，避免重复保存
   - 最多保存50个信号

9. **RF信号发送**:
   - 支持433MHz和315MHz双频发送
   - 显示已保存信号列表
   - 上下键选择信号，OK键直接发送（无发送状态页面）
   - 支持快速连续发送（点击几次就发送几次）
   - 支持12种协议
   - 使用接收时记录的脉宽进行发送

## 待实现功能

1. **信号管理**: 删除已保存信号
2. **设置页面**: 配置RF参数等

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

### Pages 模块 (页面模块化)
**职责**: 独立页面内容管理，与main.cpp解耦

**Page 基类接口**:
```cpp
class Page {
public:
    virtual void enter() = 0;           // 进入页面时初始化
    virtual void exit() {}              // 退出页面时清理
    virtual void draw() = 0;            // 绘制页面内容
    virtual bool handleButton(ButtonEvent event) = 0;  // 处理按键
    virtual const char* getTitle() = 0; // 获取页面标题
    virtual bool needsConstantRefresh(); // 是否需要每帧刷新
    virtual bool update();              // 每帧更新内部状态
};
```

**已实现页面**:
- **AboutPage**: 关于页面，显示系统信息（FPS、CPU、RAM、Flash、SDK、运行时间、电池电压）
- **SignalRxPage**: 信号接收页面，支持433/315MHz双频扫描，自动保存信号
  - 布局: 频率|编码, 协议|十六进制, 位数|脉宽, 右侧竖排状态
- **SignalTxPage**: 发送模式页面，显示已保存信号列表，OK键直接发送

**添加新页面步骤**:
1. 在 `lib/Pages/` 下创建 `XxxPage.h` 和 `XxxPage.cpp`
2. 继承 `Page` 基类，实现所有虚函数
3. 在 `main.cpp` 中添加页面对象和菜单项
4. 更新 `getPageByIndex()` 和 `getPageStateByIndex()` 函数

### RFReceiver 模块
**职责**: RF信号接收，同时监听433/315MHz

**关键特性**:
- 使用双库架构实现同时监听:
  - RCSwitch433: 433MHz接收 (GPIO1) - 本地库
  - RCSwitch315: 315MHz接收 (GPIO3) - 本地库
- 两个库各自维护独立的静态变量，互不干扰
- 中断方式接收，响应速度快
- **信号过滤**:
  - 最小位数过滤: 忽略位数<20的干扰信号 (MIN_VALID_BITS=20)
  - 2^n-1位过滤: 忽略3,7,15,31位等噪声信号
  - 全1编码过滤: 忽略全1的编码值
  - 去重窗口: 100ms内相同编码视为重复 (防止433/315混淆)
  - 冷却时间: 收到有效信号后500ms内忽略所有信号 (RECEIVE_COOLDOWN_MS=500)
- 支持多种协议: PT2262, PT2260, EV1527, HT6P20B, SC5262, HT12E等
- 调试功能: 可获取中断计数和时序数量

**协议编号映射**:
| 协议号 | 芯片型号 |
|--------|----------|
| 1 | PT2262 |
| 2 | PT2260 |
| 3 | EV1527 |
| 4 | HT6P20B |
| 5 | SC5262 |
| 6 | HT12E |
| 7 | HS2303-PT |

**接口**:
```cpp
class RFReceiver {
    // 最小有效位数 (低于此值视为干扰)
    static const unsigned int MIN_VALID_BITS = 20;
    // 去重窗口时间 (ms)
    static const unsigned long DUPLICATE_WINDOW_MS = 100;
    // 接收冷却时间 (ms)
    static const unsigned long RECEIVE_COOLDOWN_MS = 500;

    struct Signal {
        unsigned long code;     // 编码值
        unsigned int protocol;  // 协议类型
        unsigned int bits;      // 位长度
        unsigned int freq;      // 频率 (433/315)
        unsigned int pulseLength; // 脉宽 (微秒)
    };

    void begin();               // 初始化
    void startScanning();       // 开始扫描 (同时启用两个接收器)
    void stopScanning();        // 停止扫描
    void update();              // 每帧调用，检查两个频率
    bool hasNewSignal();        // 是否有新信号
    Signal getLastSignal();     // 获取最后信号
    static const char* getProtocolName(unsigned int protocol);

    // 调试功能
    unsigned long get433InterruptCount();
    unsigned long get315InterruptCount();
    void resetDebugCounters();
};
```

### RFTransmitter 模块
**职责**: RF信号发送，支持433/315MHz双频

**关键特性**:
- 使用双库架构实现双频发送:
  - RCSwitch433: 433MHz发送 (GPIO0)
  - RCSwitch315: 315MHz发送 (GPIO2)
- 根据信号频率自动选择发送器
- 支持12种协议
- 支持自定义脉宽 (使用接收时记录的脉宽)
- 默认重复发送10次

**接口**:
```cpp
class RFTransmitter {
    void begin();               // 初始化
    void send(code, bits, freq, protocol, pulseLength);  // 发送信号
    void setRepeatTransmit(int repeat);     // 设置重复次数
    bool isSending();           // 是否正在发送
};
```

### RCSwitch433 模块
**职责**: 433MHz RF信号收发 (本地库)

**关键特性**:
- 基于rc-switch库修改，使用独立的类名和静态变量
- 支持接收和发送功能
- 调试功能: 中断计数、时序数量
- 12种协议支持

**接口**:
```cpp
class RCSwitch433 {
    // 接收
    void enableReceive(int interrupt);
    void disableReceive();
    bool available();
    void resetAvailable();
    unsigned long getReceivedValue();
    void setReceiveTolerance(int nPercent);

    // 发送
    void enableTransmit(int pin);
    void disableTransmit();
    void send(unsigned long code, unsigned int length);
    void setProtocol(int nProtocol);
    void setRepeatTransmit(int repeat);

    // 调试
    static unsigned long getInterruptCount();
    static void resetInterruptCount();
    static unsigned int getLastTimingsCount();
};
```

### RCSwitch315 模块
**职责**: 315MHz RF信号收发 (本地库，独立于RCSwitch433)

**关键特性**:
- 基于rc-switch库修改，使用独立的类名和静态变量
- 与RCSwitch433统一接口风格
- 可同时工作，实现双频同时监听
- 支持接收和发送功能
- 调试功能: 中断计数、时序数量
- 参考了 https://github.com/zybaozi/RF_MANAGER 的实现方案
- 支持12种协议

### SignalStorage 模块
**职责**: 信号的JSON存储和读取

**存储格式** (`/signals.json`):
```json
[
  {
    "name": "433_1234567",
    "code": 1234567,
    "freq": 433,
    "protocol": 1,
    "bits": 24,
    "pulse": 350
  }
]
```

**接口**:
```cpp
class SignalStorage {
    struct StoredSignal {
        char name[32];
        unsigned long code;
        unsigned int freq;
        unsigned int protocol;
        unsigned int bits;
        unsigned int pulseLength;  // 脉宽
    };

    bool begin();                                   // 初始化LittleFS
    bool saveSignal(const StoredSignal& signal);    // 保存信号
    int loadSignals(StoredSignal* signals, int max);// 加载信号
    int getSignalCount();                           // 获取数量
    bool signalExists(unsigned long code);          // 检查是否存在
    static void generateName(freq, code, outName);  // 生成名称
};
```

## 性能指标

- **按键响应**: <10ms 平均延迟
- **连击速度**: 最高28次/秒 (35ms最小间隔)
- **内存使用**: 5.8% (18936字节)
- **Flash使用**: 41.9% (548756字节)
- **屏幕刷新**: 局部刷新策略，只更新变化区域
- **RF接收**: 双频同时监听，中断方式
- **RF发送**: 双频支持，12种协议

## 局部刷新机制

将屏幕分为两个区域，使用U8g2的`updateDisplayArea()`只更新变化的部分：

**屏幕区域划分** (tile坐标，每tile=8像素):
| 区域 | 像素范围 | Tile范围 | 说明 |
|------|----------|----------|------|
| 状态栏 | Y: 0-15 | tiles 0-1 | 标题、电池状态 |
| 内容区 | Y: 16-63 | tiles 2-7 | 菜单、页面内容 |

**脏区域标志**:
```cpp
bool statusBarDirty;  // 状态栏需要刷新 (电池变化时)
bool contentDirty;    // 内容区需要刷新 (按键、页面更新时)
bool fullRefresh;     // 全屏刷新 (页面切换时)
```

**刷新函数**:
- `refreshStatusBar()`: 只刷新状态栏 (传输256字节)
- `refreshContent()`: 只刷新内容区 (传输768字节)，会重绘分隔线
- `refreshAll()`: 全屏刷新 (传输1024字节)

**注意**: 分隔线在Y=16位置，属于状态栏和内容区的边界。内容区刷新时会清除此行，因此`refreshContent()`需要重绘分隔线以保持一致性。

**刷新触发条件**:
- 菜单导航 → 只刷新内容区
- 电池状态变化 → 只刷新状态栏
- 页面切换 → 全屏刷新
- 子页面更新 → 只刷新内容区

## 注意事项

1. ESP32-C3的GPIO数量有限，引脚已被充分利用
2. 按键使用外部上拉，代码中仍需配置INPUT_PULLUP
3. RF模块需要9.6V供电，由MT3608升压提供
4. 中文显示会占用较多Flash空间
5. 硬件I2C可能不稳定，建议使用软件I2C (默认)
6. 电池电压需补偿0.27V二极管压降 (关于页面已补偿)
7. 关于页面会强制每帧刷新，用于FPS测试
8. 新增页面需继承Page基类并实现所有虚函数



注意：每次修改完后都要更新 CLAUDE.md
使用中文来回答问题
