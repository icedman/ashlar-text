#include <QApplication>
#include <QDebug>
#include <QLineEdit>
#include <QVBoxLayout>

#include "Cubic.h"
#include "mainwindow.h"
#include "qt/core.h"
#include "select.h"
#include "settings.h"

Select::Select(QWidget* parent)
    : QWidget(parent)
    , updateTimer(this)
    , animateTimer(this)
{
    setProperty("id", "select");
    setWindowFlag(Qt::Popup);
    hide();

    setMinimumSize(0, 0);
    connect(&updateTimer, SIGNAL(timeout()), this, SLOT(updateSize()));
    connect(&animateTimer, SIGNAL(timeout()), this, SLOT(onAnimate()));
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
    layout->addWidget(items, 2);
    layout->addStretch(0);

    items->hide();

    setLayout(layout);
}

void Select::showEvent(QShowEvent* event)
{
    input->setText("");

    MainWindow* mw = MainWindow::instance();
    QRect parentRect = ((QWidget*)parent())->rect();
    int w = parentRect.width() * .4;
    if (w < 400) {
        w = 400;
    }
    if (w > 600) {
        w = 600;
    }

    int h = 40;

    int y = 32; // titlebar height
    if (mw->menuBar()->isVisible()) {
        y += mw->menuBar()->height() + 2;
    }

    setGeometry(mw->x() + parentRect.width() / 2 - w / 2, mw->y() + y, w, h);
    input->setFocus(Qt::ActiveWindowFocusReason);

    updateTimer.start(150);
}

void Select::hideEvent(QHideEvent* event)
{
    items->hide();
    updateTimer.stop();
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
    QApplication* app = qobject_cast<QApplication*>(QApplication::instance());
    animateTimer.stop();

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

        QScrollArea* area = items->findChild<QScrollArea*>();
        area->ensureWidgetVisible(app->focusWidget());
        area->horizontalScrollBar()->setValue(0);
        return true;
    }
    case Qt::Key_Down:
        focusNextChild();
        if (qobject_cast<QLineEdit*>(app->focusWidget())) {
            releaseKeyboard();
        }

        QScrollArea* area = items->findChild<QScrollArea*>();
        area->ensureWidgetVisible(app->focusWidget());
        area->horizontalScrollBar()->setValue(0);
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
            QApplication* app = qobject_cast<QApplication*>(QApplication::instance());
            if (qobject_cast<QScrollArea*>(app->focusWidget())) {
                focusNextChild();
            }

            QScrollArea* area = items->findChild<QScrollArea*>();
            area->ensureWidgetVisible(app->focusWidget());
            area->horizontalScrollBar()->setValue(0);
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
    QApplication* app = qobject_cast<QApplication*>(QApplication::instance());
    TouchableWidget* widget = qobject_cast<TouchableWidget*>(app->focusWidget());
    emit widget->pressed();
    hide();
}

void Select::updateSize()
{
    QScrollArea* area = items->findChild<QScrollArea*>();
    QList<TouchableWidget*> allItems = area->findChildren<TouchableWidget*>();

    bool shouldShow = allItems.size() > 1;
    int ih = input->rect().height() + 4;
    int h = ih;

    items->setVisible(shouldShow);

    if (!shouldShow) {
        targetHeight = h;
        items->hide();
        resize(width(), ih);
    } else {
        h += (allItems[1]->rect().height() - 2) * (allItems.size() - 1);
        if (h > 400) {
            h = 400;
        }
        targetHeight = h;
    }

    if (shouldShow && !animateTimer.isActive() && QWidget::height() != targetHeight) {
        animTime = -150;
        height = QWidget::height();
        animateTimer.start(25);
        // resize(width(), targetHeight);
    }
}

void Select::onAnimate()
{
    const int duration = 250;
    animTime += animateTimer.interval();
    if (animTime < 0) {
        return;
    }

    float h = Cubic::easeOut(animTime, 0, targetHeight - height, duration);
    if (animTime >= duration) {
        h = targetHeight - height;
        animateTimer.stop();
        items->show();
    }

    // qDebug() << targetHeight;
    resize(width(), h + height);
    update();
}
