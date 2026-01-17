/**
 * @file SignalStorage.h
 * @brief RF信号存储模块，使用LittleFS + ArduinoJson
 */

#ifndef SIGNAL_STORAGE_H
#define SIGNAL_STORAGE_H

#include <Arduino.h>

class SignalStorage {
public:
    // 存储的信号结构
    struct StoredSignal {
        char name[32];          // 显示名称
        unsigned long code;     // 编码值
        unsigned int freq;      // 频率 (433/315)
        unsigned int protocol;  // 协议类型
        unsigned int bits;      // 位长度
    };

    static const int MAX_SIGNALS = 50;  // 最大保存信号数量

    SignalStorage();

    /**
     * 初始化存储系统
     * @return 初始化是否成功
     */
    bool begin();

    /**
     * 保存信号
     * @param signal 要保存的信号
     * @return 是否保存成功
     */
    bool saveSignal(const StoredSignal& signal);

    /**
     * 加载所有信号
     * @param signals 信号数组
     * @param maxCount 最大加载数量
     * @return 实际加载的信号数量
     */
    int loadSignals(StoredSignal* signals, int maxCount);

    /**
     * 获取信号数量
     */
    int getSignalCount();

    /**
     * 删除指定索引的信号
     * @param index 信号索引
     * @return 是否删除成功
     */
    bool deleteSignal(int index);

    /**
     * 检查信号是否已存在（根据编码）
     * @param code 编码值
     * @return 是否存在
     */
    bool signalExists(unsigned long code);

    /**
     * 生成信号名称
     * @param freq 频率
     * @param code 编码
     * @param outName 输出名称缓冲区
     * @param maxLen 缓冲区最大长度
     */
    static void generateName(unsigned int freq, unsigned long code, char* outName, int maxLen);

private:
    static const char* STORAGE_FILE;

    /**
     * 保存所有信号到文件
     */
    bool saveToFile();

    /**
     * 从文件加载信号
     */
    bool loadFromFile();

    StoredSignal _signals[MAX_SIGNALS];
    int _signalCount;
    bool _initialized;
};

#endif // SIGNAL_STORAGE_H
