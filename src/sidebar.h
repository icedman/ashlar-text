#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QFileSystemModel>
#include <QModelIndex>
#include <QStyledItemDelegate>
#include <QThread>
#include <QTimer>
#include <QTreeView>

#include "icons.h"

class MainWindow;
class Sidebar;
class FileSystemModel;

class FileSystemModel : public QFileSystemModel {
    Q_OBJECT
public:
    FileSystemModel(QObject* parent = 0);

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;

private:
private Q_SLOTS:
    void onDirectoryLoaded(const QString& path);
};

class SidebarItemDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

    Sidebar* sidebar;
    QModelIndex hoverIndex;

public slots:
    void onHoverIndexChanged(const QModelIndex& index);
};

class Sidebar : public QTreeView {
    Q_OBJECT
public:
    Sidebar(QWidget* parent = 0);

    void animateShow();
    void animateHide();
    void setRootPath(QString path, bool deferred, bool show = false);
    void setActiveFile(QString path);
    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>()) override;

    void trigger(int button);

    QStringList allFiles();

    QString root() { return rootPath; }

    FileSystemModel* fileModel;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

    QTimer animateTimer;
    QTimer clickTimer;
    QString rootPath;
    QStringList excludeFiles;
    QStringList excludeFolders;
    SidebarItemDelegate* itemDelegate;

    float animTime;
    float startWidth;
    float targetWidth;
    float lastWidth;

    bool showOnLoad;

    void fetchFiles(QFileSystemModel* m, QModelIndex index, QStringList& res);

private Q_SLOTS:
    void _setRootPath();

    void onAnimate();

signals:
    void hoverIndexChanged(const QModelIndex& index);
};

#endif // SIDEBAR_H