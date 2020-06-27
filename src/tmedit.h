#ifndef TM_EDIT_H
#define TM_EDIT_H

#include <QCompleter>
#include <QFileSystemWatcher>
#include <QPlainTextEdit>
#include <QTextBlock>
#include <QTextCursor>
#include <QTimer>
#include <QWidget>

class Editor;

class Overlay : public QWidget {
    Q_OBJECT
public:
    Overlay(QWidget* parent = nullptr);

    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;

    QPixmap buffer;

    QTimer updateTimer;
    bool cursorOn;

private Q_SLOTS:
    void updateCursor();
};

class TextmateEdit : public QPlainTextEdit {
    Q_OBJECT
public:
    TextmateEdit(QWidget* parent = nullptr);

    QTextBlock _firstVisibleBlock() { return firstVisibleBlock(); }

    QRectF _blockBoundingGeometry(QTextBlock& block)
    {
        return blockBoundingGeometry(block);
    }

    QPointF _contentOffset() { return contentOffset(); }

public:
    void addExtraCursor(QTextCursor cursor = QTextCursor());
    void removeExtraCursors();
    void updateExtraCursors(QKeyEvent* e);
    QList<QTextCursor> extraCursors;

    void paintToBuffer();
    QPointF offset() { return _offset; }

private:
    bool completerKeyPressEvent(QKeyEvent* e);
    void paintEvent(QPaintEvent* e) override;
    void mousePressEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* e) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void wheelEvent(QWheelEvent* e) override;

    Overlay* overlay;
    Editor* editor;

    QPointF _offset;
    QPointF scrollDelta;
    QPointF scrollVelocity;
    QTimer updateTimer;

    QCompleter* completer;

private Q_SLOTS:
    void updateScrollDelta();
    void insertCompletion(const QString& completion);
};

#endif // TM_EDIT_H