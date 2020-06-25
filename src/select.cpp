#include <QDebug>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QApplication>

#include "mainwindow.h"
#include "qt/core.h"
#include "select.h"
#include "settings.h"

Select::Select(QWidget* parent)
    : QWidget(parent)
{
    setProperty("id", "select");
    setWindowFlag(Qt::Popup);
    hide();
}

void Select::setup()
{
    MainWindow* mw = MainWindow::instance();

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setMargin(2);
    layout->setSpacing(4);

    input = qobject_cast<QLineEdit*>(mw->js()->create("select::input", "TextInput", true)->widget());
    input->installEventFilter(this);

    items = qobject_cast<QWidget*>(mw->js()->create("select::items", "View", true)->widget());

    layout->addWidget(input, 1);
    layout->addWidget(items, 1);

    setLayout(layout);
}

void Select::showEvent(QShowEvent* event)
{
    MainWindow* mw = MainWindow::instance();
    QRect parentRect = ((QWidget*)parent())->rect();
    int w = parentRect.width() * .4;
    int h = parentRect.height() * .6;
    if (w < 400) {
        w = 400;
    }
    if (w > 600) {
        w = 600;
    }
    if (h < 400) {
        h = 400;
    }
    if (h > 400) {
        h = 400;
    }

    int y = 32; // titlebar height
    if (mw->menuBar()->isVisible()) {
        y += mw->menuBar()->height() + 2;
    }

    setGeometry(mw->x() + parentRect.width() / 2 - w / 2, mw->y() + y, w, h);
    setMouseTracking(true);

    input->setFocus(Qt::ActiveWindowFocusReason);
}

void Select::hideEvent(QHideEvent* event)
{
    releaseKeyboard();
}

void Select::paintEvent(QPaintEvent* event)
{
    // QWidget::paintEvent(event);

    QColor bgColor;
    if (!theme_color(MainWindow::instance()->theme, "editor.background", bgColor)) {
        bgColor = QColor(0xc0, 0xc0, 0xc0);
    }

    bgColor = bgColor.darker(110);

    QRect rect = event->rect();
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect, bgColor);
}

void Select::mousePressEvent(QMouseEvent* event)
{
    QWidget::mousePressEvent(event);
    if (!rect().contains(event->x(), event->y())) {
        hide();
    }
}

void Select::keyPressEvent(QKeyEvent* e)
{
    QApplication *app = qobject_cast<QApplication*>(QApplication::instance()); 
    
    switch (e->key()) {
    case Qt::Key_Escape:
        hide();
        return true;
    case Qt::Key_Return:
        trigger();
        return true;
    case Qt::Key_Up: {
        focusPreviousChild();
        if (qobject_cast<QScrollArea*>(app->focusWidget())) {
            focusPreviousChild();
            releaseKeyboard();
        }
        return true;
    }
    case Qt::Key_Down:
        focusNextChild();
        return true;
    }
    
    input->setFocus(Qt::ActiveWindowFocusReason);
    input->setText(input->text() + e->text());
    releaseKeyboard();
    // QWidget::keyPressEvent(e);
}

bool Select::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        switch (keyEvent->key()) {
        case Qt::Key_Escape:
            hide();
            return true;
        case Qt::Key_Return:
            trigger();
            return true;
        case Qt::Key_Up:
            return true;
        case Qt::Key_Down: {
            focusNextChild(); // skip the scrollarea
            QApplication *app = qobject_cast<QApplication*>(QApplication::instance());
            if (qobject_cast<QScrollArea*>(app->focusWidget())) {
                focusNextChild();
            }
            grabKeyboard();
            return true;
        }
        }
    }

    // standard event processing
    return QObject::eventFilter(obj, event);
}

void Select::trigger()
{
    QApplication *app = qobject_cast<QApplication*>(QApplication::instance());
    TouchableWidget *widget = qobject_cast<TouchableWidget*>(app->focusWidget());
    emit widget->pressed();
    hide();
}

