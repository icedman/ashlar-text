#ifndef SELECT_H
#define SELECT_H

#include <QtWidgets>

class QLineEdit;
class Select : public QWidget {
    Q_OBJECT
public:
    Select(QWidget* parent = 0);
    void setup();
    void trigger();

protected:
    bool eventFilter(QObject* obj, QEvent* event);

private:
    void paintEvent(QPaintEvent*) override;
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* e) override;

    QLineEdit* input;
    QWidget* items;
};

#endif // SELECT_H