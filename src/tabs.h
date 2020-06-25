#ifndef TABS_H
#define TABS_H

#include <QTabBar>

class Editor;
class Tabs : public QTabBar {
    Q_OBJECT
public:
    Tabs(QWidget* parent = 0);

    int findTabByName(QString name);
    int findTabByPath(QString path);

    Editor* editor(int idx);
};

#endif // TABS_H