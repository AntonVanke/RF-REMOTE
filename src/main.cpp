#include <Arduino.h>
#include <BatteryMonitor.h>
#include "pin_config.h"

BatteryMonitor battery;

void setup() {
  Serial.begin(115200);
  battery.begin();
}

void loop() {
  float voltage = battery.readVoltage();

  Serial.print("电池电压: ");
  Serial.print(voltage, 2);
  Serial.println("V");

  delay(1000);
}
