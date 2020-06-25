#ifndef SIDEBAR_H
#define SIDEBAR_H

#include <QFileSystemModel>
#include <QModelIndex>
#include <QTimer>
#include <QTreeView>

#include "icons.h"

class MainWindow;

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
    void setRootPath(QString path, bool deferred);
    void setActiveFile(QString path);
    void dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles = QVector<int>()) override;

    FileSystemModel* fileModel;

protected:
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);

private:
    QTimer animateTimer;
    QTimer updateTimer;
    QString rootPath;

    float animTime;
    float width;
    float targetWidth;

private Q_SLOTS:
    void onAnimate();
    void _setRootPath();
};

#endif // SIDEBAR_H