#include "tabs.h"
#include "editor.h"

#include <QToolButton>

Tabs::Tabs(QWidget* parent)
    : QTabBar(parent)
{
}

int Tabs::findTabByName(QString name)
{
    for (int i = 0; i < count(); i++) {
        if (tabText(i) == name) {
            return i;
        }
    }
    return -1;
}

int Tabs::findTabByPath(QString path)
{
    for (int i = 0; i < count(); i++) {
        QVariant data = tabData(i);
        Editor* editor = qvariant_cast<Editor*>(data);
        if (editor->fileName == path) {
            return i;
        }
    }
    return -1;
}

Editor* Tabs::editor(int idx)
{
    if (idx == -1 || idx >= count()) {
        return NULL;
    }

    QVariant data = tabData(idx);
    return qvariant_cast<Editor*>(data);
}

int Tabs::findPreviewTab()
{
    for (int i = 0; i < count(); i++) {
        QVariant data = tabData(i);
        Editor* editor = qvariant_cast<Editor*>(data);
        if (editor->isPreview()) {
            return i;
        }
    }
    return -1;
}

void Tabs::removePreviewTag()
{
    int previewIdx = findPreviewTab();
    if (previewIdx != -1) {
        Editor* e = editor(previewIdx);
        setTabIcon(previewIdx, QIcon());
        e->setPreview(false);
    }
}