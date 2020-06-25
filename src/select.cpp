#include <QDebug>
#include <QVBoxLayout>
#include <QLineEdit>

#include "qt/core.h"
#include "settings.h"
#include "select.h"
#include "mainwindow.h"

Select::Select(QWidget *parent) :
    QWidget(parent)
{
    setProperty("id", "select");
    hide();
}

void Select::showEvent(QShowEvent *event)
{
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
   
    int y = 0; 
    MainWindow *mw = MainWindow::instance();
    if (mw->menuBar()->isVisible()) {
        y += mw->menuBar()->height() + 2;
    }
    
    setGeometry(parentRect.width()/2 - w/2, y, w, h);        
    setMouseTracking(true);
    
    grabMouse();
    
    input->setFocus(Qt::ActiveWindowFocusReason);
}

void Select::setup()
{
    MainWindow *mw = MainWindow::instance();
    
    QVBoxLayout *layout = new QVBoxLayout(this);    
    layout->setMargin(2);
    layout->setSpacing(4);

    input = qobject_cast<QLineEdit*>(mw->js()->create("select::input", "TextInput", true)->widget());
    input->installEventFilter(this);
    
    items = qobject_cast<QWidget*>(mw->js()->create("select::items", "View", true)->widget());
 
    layout->addWidget(input, 1);
    layout->addWidget(items, 1);
    
    setLayout(layout);
}

void Select::hideEvent(QHideEvent *event)
{
    releaseMouse();
}

void Select::paintEvent(QPaintEvent *event) {
    // QWidget::paintEvent(event);
    
    QColor bgColor;
    if (!theme_color(MainWindow::instance()->theme, "editor.background", bgColor)) {
        bgColor = QColor(0xc0,0xc0,0xc0);
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

bool Select::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::KeyPress) {
         QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
         if (keyEvent->key() == Qt::Key_Escape) {
             hide();
         }
         if (keyEvent->key() == Qt::Key_Return) {
             hide();
             return true;
         }
     }
     
     // standard event processing
     return QObject::eventFilter(obj, event);
}
