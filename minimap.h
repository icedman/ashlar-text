#ifndef MINIMAP_H
#define MINIMAP_H

#include <QScrollBar>
#include <QTimer>

class Editor;

class MiniMap : public QScrollBar {
    Q_OBJECT

public:
    MiniMap(QWidget* parent = 0);

    QColor backgroundColor;

    Editor* editor;

    void setSizes(size_t firstVisible, int visible, size_t val, size_t max);

    size_t value;
    size_t maximum;
    size_t firstVisible;
    int visibleLines;

    float offsetY;
    float scrollToY;

    QPixmap buffer;

private:
    void scrollByMouseY(float y);

private Q_SLOTS:
    void updateScroll();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QTimer updateTimer;
};

#endif // MINIMAP_H