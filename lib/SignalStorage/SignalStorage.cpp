/**
 * @file SignalStorage.cpp
 * @brief RF信号存储模块实现
 */

#include "SignalStorage.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <esp32-hal-log.h>

static const char* TAG = "Storage";

const char* SignalStorage::STORAGE_FILE = "/signals.json";

SignalStorage::SignalStorage()
    : _signalCount(0)
    , _initialized(false)
{
    memset(_signals, 0, sizeof(_signals));
}

bool SignalStorage::begin() {
    ESP_LOGI(TAG, "初始化信号存储...");

    if (!LittleFS.begin(true)) {  // true = 格式化如果挂载失败
        ESP_LOGE(TAG, "LittleFS挂载失败!");
        return false;
    }

    ESP_LOGI(TAG, "LittleFS挂载成功");

    // 尝试加载已保存的信号
    if (!loadFromFile()) {
        ESP_LOGW(TAG, "无已保存的信号或加载失败，从空白开始");
        _signalCount = 0;
    }

    ESP_LOGI(TAG, "已加载 %d 个信号", _signalCount);
    _initialized = true;
    return true;
}

bool SignalStorage::saveSignal(const StoredSignal& signal) {
    if (!_initialized) {
        ESP_LOGE(TAG, "存储未初始化");
        return false;
    }

    // 检查是否已存在
    if (signalExists(signal.code)) {
        ESP_LOGW(TAG, "信号已存在: %lu", signal.code);
        return false;
    }

    // 检查是否已满
    if (_signalCount >= MAX_SIGNALS) {
        ESP_LOGW(TAG, "存储已满，最多 %d 个信号", MAX_SIGNALS);
        return false;
    }

    // 添加信号
    _signals[_signalCount] = signal;
    _signalCount++;

    ESP_LOGI(TAG, "保存信号: %s (编码:%lu)", signal.name, signal.code);

    // 保存到文件
    return saveToFile();
}

int SignalStorage::loadSignals(StoredSignal* signals, int maxCount) {
    if (!_initialized) return 0;

    int count = min(maxCount, _signalCount);
    memcpy(signals, _signals, count * sizeof(StoredSignal));
    return count;
}

int SignalStorage::getSignalCount() {
    return _signalCount;
}

bool SignalStorage::deleteSignal(int index) {
    if (!_initialized || index < 0 || index >= _signalCount) {
        return false;
    }

    // 移动后续信号
    for (int i = index; i < _signalCount - 1; i++) {
        _signals[i] = _signals[i + 1];
    }
    _signalCount--;

    ESP_LOGI(TAG, "删除信号索引: %d", index);

    return saveToFile();
}

bool SignalStorage::signalExists(unsigned long code) {
    for (int i = 0; i < _signalCount; i++) {
        if (_signals[i].code == code) {
            return true;
        }
    }
    return false;
}

void SignalStorage::generateName(unsigned int freq, unsigned long code, char* outName, int maxLen) {
    snprintf(outName, maxLen, "%d_%lu", freq, code);
}

bool SignalStorage::saveToFile() {
    File file = LittleFS.open(STORAGE_FILE, "w");
    if (!file) {
        ESP_LOGE(TAG, "无法打开文件写入: %s", STORAGE_FILE);
        return false;
    }

    // 创建JSON文档
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (int i = 0; i < _signalCount; i++) {
        JsonObject obj = arr.add<JsonObject>();
        obj["name"] = _signals[i].name;
        obj["code"] = _signals[i].code;
        obj["freq"] = _signals[i].freq;
        obj["protocol"] = _signals[i].protocol;
        obj["bits"] = _signals[i].bits;
    }

    // 序列化到文件
    size_t bytesWritten = serializeJson(doc, file);
    file.close();

    ESP_LOGD(TAG, "保存 %d 个信号到文件, 写入 %d 字节", _signalCount, bytesWritten);
    return bytesWritten > 0;
}

bool SignalStorage::loadFromFile() {
    File file = LittleFS.open(STORAGE_FILE, "r");
    if (!file) {
        ESP_LOGD(TAG, "文件不存在: %s", STORAGE_FILE);
        return false;
    }

    // 解析JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        ESP_LOGE(TAG, "JSON解析失败: %s", error.c_str());
        return false;
    }

    // 读取信号数组
    JsonArray arr = doc.as<JsonArray>();
    _signalCount = 0;

    for (JsonObject obj : arr) {
        if (_signalCount >= MAX_SIGNALS) break;

        const char* name = obj["name"] | "";
        strncpy(_signals[_signalCount].name, name, sizeof(_signals[0].name) - 1);
        _signals[_signalCount].code = obj["code"] | 0UL;
        _signals[_signalCount].freq = obj["freq"] | 433;
        _signals[_signalCount].protocol = obj["protocol"] | 1;
        _signals[_signalCount].bits = obj["bits"] | 24;

        _signalCount++;
    }

    return true;
}
