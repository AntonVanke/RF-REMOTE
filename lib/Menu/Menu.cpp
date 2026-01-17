/**
 * @file Menu.cpp
 * @brief 菜单管理模块实现 (优化版)
 */

#include "Menu.h"

// 预定义序号字符串，避免每帧sprintf
static const char* INDEX_STRINGS[] = {"1.", "2.", "3.", "4.", "5.", "6.", "7.", "8."};

Menu::Menu(U8G2* display, const char** items, int itemCount)
    : _display(display), _menuItems(items), _itemCount(itemCount), _currentSelection(0) {}

void Menu::draw() {
    // 优化：只在开始时设置一次字体
    _display->setFont(u8g2_font_wqy12_t_gb2312);

    for (int i = 0; i < _itemCount; i++) {
        int yPos = MENU_START_Y + i * MENU_ITEM_HEIGHT + 12;

        // 绘制序号 (使用预定义字符串，避免sprintf)
        _display->drawStr(2, yPos, INDEX_STRINGS[i]);

        // 绘制选中标记
        if (i == _currentSelection) {
            _display->drawStr(16, yPos, ">");
        }

        // 绘制菜单文字
        _display->drawUTF8(28, yPos, _menuItems[i]);
    }
}

void Menu::next() {
    _currentSelection = (_currentSelection + 1) % _itemCount;
}

void Menu::previous() {
    _currentSelection = (_currentSelection - 1 + _itemCount) % _itemCount;
}

int Menu::getCurrentSelection() {
    return _currentSelection;
}
