#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <BatteryMonitor.h>
#include "pin_config.h"

// 初始化U8g2 (使用软件I2C, SSD1306 128x64)
// 参数: 旋转方向, 时钟引脚, 数据引脚, 复位引脚
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, OLED_SCL_PIN, OLED_SDA_PIN, U8X8_PIN_NONE);

BatteryMonitor battery;

// 菜单配置
const int MENU_ITEMS_COUNT = 2;
const char* menuItems[] = {
    "信号接收",
    "发送模式"
};
int currentSelection = 0;  // 当前选中项 (0-1)

// 绘制主菜单
void drawMenu() {
    u8g2.clearBuffer();

    // 绘制标题栏 (黄色区域: 0-15像素)
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);  // 12x12中文字体
    u8g2.drawUTF8(0, 12, "RF遥控器");

    // 显示页码 (右上角)
    u8g2.setFont(u8g2_font_6x12_tf);  // 小号英文字体
    char pageInfo[8];
    sprintf(pageInfo, "[%d/%d]", currentSelection + 1, MENU_ITEMS_COUNT);
    u8g2.drawStr(128 - u8g2.getStrWidth(pageInfo), 12, pageInfo);

    // 绘制分隔线
    u8g2.drawHLine(0, 16, 128);

    // 绘制菜单项 (蓝色区域: 16-63像素)
    u8g2.setFont(u8g2_font_wqy12_t_gb2312);  // 12x12中文字体
    for (int i = 0; i < MENU_ITEMS_COUNT; i++) {
        int yPos = 18 + i * 16 + 12;  // 起始18 + 间距16 + 字体基线调整

        // 绘制选中标记
        if (i == currentSelection) {
            u8g2.drawStr(2, yPos, ">");
        }

        // 绘制菜单文字
        u8g2.drawUTF8(14, yPos, menuItems[i]);
    }

    u8g2.sendBuffer();
}

// I2C扫描函数
void scanI2C() {
  Serial.println("\n开始扫描I2C设备...");
  Serial.print("SDA引脚: ");
  Serial.print(OLED_SDA_PIN);
  Serial.print(", SCL引脚: ");
  Serial.println(OLED_SCL_PIN);

  byte count = 0;
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);

  for (byte i = 1; i < 127; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("发现I2C设备，地址: 0x");
      Serial.println(i, HEX);
      count++;
    }
  }

  Serial.print("扫描完成，找到 ");
  Serial.print(count);
  Serial.println(" 个设备\n");
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // 等待串口初始化

  Serial.println("\n==============================");
  Serial.println("RF遥控器启动中...");
  Serial.println("==============================");

  // 扫描I2C设备
  scanI2C();

  // 初始化OLED
  Serial.println("初始化OLED显示屏...");
  if (!u8g2.begin()) {
    Serial.println("错误: OLED初始化失败!");
  } else {
    Serial.println("OLED初始化成功");
  }

  u8g2.enableUTF8Print();  // 启用UTF-8中文支持

  // 初始化电池监测
  battery.begin();

  // 显示初始菜单
  Serial.println("绘制菜单界面...");
  drawMenu();

  Serial.println("RF遥控器已启动完成");
  Serial.println("==============================\n");
}

void loop() {
  // 暂时空循环，等待后续添加按键逻辑
  delay(100);
}
