#ifndef GUTTER_H
#define GUTTER_H

#include "highlighter.h"
#include <QWidget>

class Editor;

struct gutter_info_t {
    int position;
    int number;
    bool foldable;
};

class Gutter : public QWidget {
    Q_OBJECT

public:
    Gutter(QWidget* parent = 0);

    QVector<gutter_info_t> lineNumbers;
    QColor lineNumberColor;
    QColor backgroundColor;
    QFont font;

    Editor* editor;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
};

#endif // GUTTERH