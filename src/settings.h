#ifndef THEME_H
#define THEME_H

#include "theme.h"

struct default_colors_t {
    QColor bg;
    QColor fg;
    QColor widgetBg;
    QColor widgetFg;
    QColor hoverBg;
    QColor hoverFg;
    QColor tabBg;
    QColor tabFg;
    QColor tabHoverBg;
    QColor tabHoverFg;
    QColor treeBg;
    QColor treeFg;
    QColor treeHoverBg;
    QColor treeHoverFg;
    QColor statusBg;
    QColor statusFg;
    QColor statusHoverBg;
    QColor statusHoverFg;
    QColor menuBg;
    QColor menuFg;
    QColor menuHoverBg;
    QColor menuHoverFg;
};

bool color_is_dark(color_info_t& color);

bool theme_is_dark(theme_ptr theme);

bool theme_color(theme_ptr theme, std::string name, QColor& qcolor);

/*
bool theme_scrollbar(theme_ptr theme, std::string name, QWidget& scrollbar);
bool theme_splitter(theme_ptr theme, std::string name, QWidget& splitter);
bool theme_sidebar(theme_ptr theme, std::string name, QWidget& tree);
bool theme_statusbar(theme_ptr theme, std::string name, QWidget& statusbar);
bool theme_tabbar(theme_ptr theme, std::string name, QWidget& tabbar);
*/

bool theme_application(theme_ptr theme, default_colors_t &colors);

#endif // THEME_H