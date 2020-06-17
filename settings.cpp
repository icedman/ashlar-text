#include <QColor>
#include <QWidget>
#include <QDebug>
#include <QFile>
#include <QApplication>
#include <QRegularExpression>
#include <QStatusBar>

#include "settings.h"
#include "mainwindow.h"

bool theme_is_dark(theme_ptr theme) {
  color_info_t clr;
  theme->theme_color("editor.background", clr);
  return color_is_dark(clr);
}

bool color_is_dark(color_info_t& color)
{
    return 0.30 * color.red + 0.59 * color.green + 0.11 * color.blue < 0.5;
}

bool theme_color(theme_ptr theme, std::string name, QColor& qcolor)
{
    color_info_t clr;
    theme->theme_color(name, clr);
    if (clr.is_blank()) {
        return false;
    }
    qcolor.setRed(clr.red * 255);
    qcolor.setGreen(clr.green * 255);
    qcolor.setBlue(clr.blue * 255);
    return true;
}

bool theme_application(theme_ptr theme)
{
  QString basePath = QCoreApplication::applicationDirPath();
  QFile file(basePath + "/css/style.css");
  if (!file.open(QFile::ReadOnly | QFile::Text)) {
    return true;
  }

  QString css = "";

  QColor bgColor;
  if (!theme_color(theme, "editor.background", bgColor)) {
      // return false;
  }
  QColor fgColor;
  if (!theme_color(theme, "editor.foreground", fgColor)) {
      // return false;
  }

  QColor fgDisabledColor = fgColor.darker(180);
  
  // widgets in general
  QColor widgetBg = bgColor.darker(105);
  QColor widgetFg = fgColor.darker(110);

  // tree
  QColor treeBg = bgColor.darker(110);
  QColor treeFg = fgColor.darker(115);
  QColor itemActiveBg;
  QColor itemActiveFg;
  QColor itemHoverBg;
  QColor itemHoverFg;
  if (!theme_color(theme, "list.activeSelectionBackground", itemActiveBg)) {
    itemActiveBg = bgColor.light(150);
  }
  if (!theme_color(theme, "list.activeSelectionForeground", itemActiveFg)) {
    itemActiveFg = fgColor.light(150);
  }
  itemHoverBg = itemActiveBg.darker(120);
  itemHoverFg = itemActiveFg.darker(110);

  // tab
  QColor tabBg = widgetBg.darker(105);
  QColor tabFg = widgetFg.darker(110);
  QColor tabItemBg;
  QColor tabItemFg;
  QColor tabItemActiveBg;
  QColor tabItemActiveFg;
  QColor tabItemHoverBg;
  QColor tabItemHoverFg;
  // The tabs

  if (!theme_color(theme, "tab.activeBackground", tabItemActiveBg) ||
    !theme_color(theme, "tab.activeForeground", tabItemActiveFg)) {
    tabItemActiveBg = itemActiveBg;
    tabItemActiveFg = itemActiveFg;
  }
  if (!theme_color(theme, "tab.inactiveBackground", tabItemBg) ||
    !theme_color(theme, "tab.inactiveForeground", tabItemFg)) {
    tabItemBg = itemActiveBg;
    tabItemFg = itemActiveFg;
  }
  tabItemHoverBg = tabItemActiveBg.darker(110);
  tabItemHoverFg = tabItemActiveFg.darker(115);

  // status bar
  QColor statusBg;
  QColor statusFg;
  
  if (!theme_color(theme, "statusBar.background", statusBg) ||
       !theme_color(theme, "statusBar.foreground", statusFg)) {
      statusBg = treeBg.darker(150);
      statusBg = treeFg;
  }

  // input
  QColor inputBg;
  QColor inputFg;
  if (!theme_color(theme, "input.background", inputBg) ||
    !theme_color(theme, "input.foreground", inputFg)) {
    inputBg = widgetBg;
    inputFg = widgetFg;
  }

  QRegularExpression regex(QStringLiteral("\\@[a-zA-Z]*"));

  QMap<QString, QString> colors;
  colors["@bgColor"] = bgColor.name();
  colors["@fgColor"] = fgColor.name();
  colors["@widgetBg"] = widgetBg.name();
  colors["@widgetFg"] = widgetFg.name();
  colors["@fgDisabledColor"] = fgDisabledColor.name();

  colors["@scrollBg"] = bgColor.darker(105).name();
  colors["@scrollHandleBg"] = bgColor.lighter(150).name();
  colors["@scrollHandleHoverBg"] = bgColor.lighter(180).name();
  colors["@splitterBg"] = bgColor.name();

  colors["@treeBg"] = treeBg.name();
  colors["@treeFg"] = treeFg.name();
  colors["@treeItemActiveBg"] = itemActiveBg.name();
  colors["@treeItemHoverBg"] = itemHoverBg.name();
  colors["@treeItemActiveFg"] = itemActiveFg.name();
  colors["@treeItemHoverFg"] = itemHoverFg.name();

  colors["@tabBg"] = tabBg.name();
  colors["@tabFg"] = tabFg.name();
  colors["@tabItemBg"] = tabItemBg.name();
  colors["@tabItemFg"] = tabItemFg.name();
  colors["@tabItemActiveBg"] = tabItemActiveBg.name();
  colors["@tabItemHoverBg"] = tabItemHoverBg.name();
  colors["@tabItemActiveFg"] = tabItemActiveFg.name();
  colors["@tabItemHoverFg"] = tabItemHoverFg.name();

  colors["@inputBg"] = inputBg.name();
  colors["@inputFg"] = inputFg.name();
  colors["@inputBorder"] = inputBg.darker(130).name();

  colors["@statusBg"] = statusBg.name();
  colors["@statusFg"] = statusFg.name();

  // qDebug () << statusBg.name();

  QTextStream in(&file);
  while (!in.atEnd())
  {
    QString line = in.readLine();
    QRegularExpressionMatch match = regex.match(line);
    if (match.hasMatch()) {
      QString color = colors[match.captured()];
      line = line.replace(match.capturedStart(), match.capturedLength(), color);
    }
    // qDebug() << line;
    css += line + "\n";
  }
  file.close();
  
  qobject_cast<QApplication*>(QApplication::instance())->setStyleSheet(css);

  return true;
}