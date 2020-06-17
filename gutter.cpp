#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QtWidgets>

#include <iostream>

#include "editor.h"
#include "gutter.h"

Gutter::Gutter(QWidget* parent)
    : QWidget(parent)
{
}

static bool isFolded(QTextBlock& block)
{
    HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
    if (blockData) {
        return blockData->folded;
    }
    return false;
}

void Gutter::paintEvent(QPaintEvent* event)
{
    TextmateEdit* tm = editor->editor;

    // std::cout << "gutter paint" << std::endl;

    QPainter p(this);
    p.fillRect(event->rect(), backgroundColor);
    p.setPen(lineNumberColor);
    p.setFont(font);
    int fh = QFontMetrics(font).height();
    int fw = QFontMetrics(font).width('w');

    // find cursor
    QTextCursor cursor = tm->textCursor();
    QTextBlock cursorBlock = cursor.block();
    int cursorPosition = cursor.position();
    int cursorLine = -1;
    if (cursorBlock.isValid()) {
        cursorLine = cursorBlock.firstLineNumber();
    }

    // the numbers
    foreach (block_info_t ln, lineNumbers) {
        if (ln.number == cursorLine) {
            p.fillRect(QRect(0, ln.position + fh, width(), fh), backgroundColor.lighter(150));
        }
        p.drawText(0, ln.position, width() - 4 - (fw * 1), fh, Qt::AlignRight, QString::number(ln.number));
    }

    // the brackets
    foreach (block_info_t ln, lineNumbers) {
        QTextDocument* doc = tm->document();
        QTextBlock block = doc->findBlockByNumber(ln.number - 1);
        if (!block.isValid()) {
            continue;
        }

        HighlightBlockData* blockData = reinterpret_cast<HighlightBlockData*>(block.userData());
        if (blockData && ln.foldable) {
            // std::cout << blockData->brackets[0].char_idx << std::endl;
            p.save();
            if (isFolded(block)) {
                p.drawText(0, ln.position, width() - 4, fh, Qt::AlignRight, "-");
            } else {
                p.drawText(0, ln.position + 4, width() - 4, fh, Qt::AlignRight, "^");
            }
            p.restore();
        }
    }
}

void Gutter::mousePressEvent(QMouseEvent* event)
{
    int fh = QFontMetrics(font).lineSpacing();
    int fw = QFontMetrics(font).width('w');
    int foldIndicatorWidth = fw + 4;
    int xofs = width() - foldIndicatorWidth;
    int lineNo = -1;
    int ys = event->pos().y();
    if (event->pos().x() > xofs) {
        foreach (auto ln, lineNumbers)
            if (ln.position < ys && (ln.position + fh) > ys) {
                if (ln.foldable) {
                    lineNo = ln.number;
                }
                break;
            }
    }
    if (lineNo >= 0) {
        if (editor) {
            editor->toggleFold(lineNo);
        }
    }
}