/**
 * @file Menu.h
 * @brief 菜单管理模块
 *
 * 管理菜单项的显示和导航逻辑
 */

#ifndef MENU_H
#define MENU_H

#include <U8g2lib.h>

class Menu {
public:
    /**
     * @brief 构造函数
     * @param display U8G2实例指针
     * @param items 菜单项数组
     * @param itemCount 菜单项数量
     */
    Menu(U8G2* display, const char** items, int itemCount);

    /**
     * @brief 绘制菜单区域
     */
    void draw();

    /**
     * @brief 选择下一项
     */
    void next();

    /**
     * @brief 选择上一项
     */
    void previous();

    /**
     * @brief 获取当前选中项索引
     * @return 选中项索引 (0-based)
     */
    int getCurrentSelection();

private:
    U8G2* _display;
    const char** _menuItems;
    int _itemCount;
    int _currentSelection;

    static const int MENU_START_Y = 18;
    static const int MENU_ITEM_HEIGHT = 16;
};

#endif // MENU_H
