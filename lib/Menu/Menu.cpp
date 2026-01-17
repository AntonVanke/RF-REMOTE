/**
 * @file Menu.cpp
 * @brief 菜单管理模块实现
 */

#include "Menu.h"

Menu::Menu(U8G2* display, const char** items, int itemCount)
    : _display(display), _menuItems(items), _itemCount(itemCount), _currentSelection(0) {}

void Menu::draw() {
    _display->setFont(u8g2_font_wqy12_t_gb2312);

    for (int i = 0; i < _itemCount; i++) {
        int yPos = MENU_START_Y + i * MENU_ITEM_HEIGHT + 12;

        // 绘制序号
        _display->setFont(u8g2_font_6x10_tf);
        char indexStr[4];
        sprintf(indexStr, "%d.", i + 1);
        _display->drawStr(2, yPos, indexStr);

        // 绘制选中标记
        _display->setFont(u8g2_font_wqy12_t_gb2312);
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
