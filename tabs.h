#ifndef TABS_H
#define TABS_H

#include <QTabBar>

class Tabs : public QTabBar {
    Q_OBJECT
public:
    Tabs(QWidget* parent = 0);
};

#endif // TABS_H