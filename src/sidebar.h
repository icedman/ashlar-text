#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QFileSystemModel>
#include <QModelIndex>
#include <QTimer>
#include <QThread>
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

class Sidebar : public QTreeView {
    Q_OBJECT
public:
    Sidebar(QWidget* parent = 0);

    void animateShow();
    void animateHide();
    void setRootPath(QString path, bool deferred, bool show = false);
    void setActiveFile(QString path);
    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>()) override;

    QStringList allFiles();
    
    FileSystemModel* fileModel;

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);

    QTimer animateTimer;
    QTimer clickTimer;
    QString rootPath;
    QStringList excludeFiles;
    QStringList excludeFolders;
    
    float animTime;
    float width;
    float targetWidth;
    
    bool showOnLoad;

    void fetchFiles(QFileSystemModel *m, QModelIndex index, QStringList &res);
    
private Q_SLOTS:    
    void _setRootPath();
   
    void onAnimate();;
};

#endif // SIDEBAR_H