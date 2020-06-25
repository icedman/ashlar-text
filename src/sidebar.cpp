#include <QDebug>
#include <QDir>
#include <QSplitter>

#include <iostream>

#include "mainwindow.h"
#include "sidebar.h"

FileSystemModel::FileSystemModel(QObject* parent)
    : QFileSystemModel(parent)
{
    setOptions(QFileSystemModel::DontUseCustomDirectoryIcons);
    connect(this, SIGNAL(directoryLoaded(const QString&)), this, SLOT(onDirectoryLoaded(const QString&)));
    // void fileRenamed(const QString &path, const QString &oldName, const QString &newName);
}

void FileSystemModel::onDirectoryLoaded(const QString& path)
{
    QModelIndex dirIndex = index(path);
    emit dataChanged(dirIndex, QModelIndex());
}

QVariant FileSystemModel::data(const QModelIndex& index, int role) const
{
    if (role == Qt::DecorationRole) {
        MainWindow *mw = MainWindow::instance();
        QFileInfo info = fileInfo(index);
        QString fileName = info.fileName();
        if (info.isFile()) {
            QString suffix = info.suffix();
            return icon_for_file(mw->icons, fileName, suffix, mw->extensions);
        } else {
            bool expanded = ((QTreeView*)parent())->isExpanded(index);
            ;
            return icon_for_folder(mw->icons, fileName, expanded, mw->extensions);
        }
    }

    return QFileSystemModel::data(index, role);
}

Sidebar::Sidebar(QWidget* parent)
    : QTreeView(parent)
    , fileModel(0)
    , animateTimer(this)
    , updateTimer(this)
{
    setHeaderHidden(true);
    setAnimated(true);
    hide();

    setMinimumSize(0, 0);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    updateTimer.setSingleShot(true);
    
    connect(&animateTimer, SIGNAL(timeout()), this, SLOT(onAnimate()));
}

void Sidebar::setRootPath(QString path, bool deferred)
{
    if (fileModel || path.isEmpty()) {
        return;
    }

    MainWindow* main = MainWindow::instance();

    fileModel = new FileSystemModel(this);
    rootPath = path;

    setModel(fileModel);

    for (int i = 1; i < fileModel->columnCount(); i++) {
        hideColumn(i);
    }

    if (deferred) {
        QTimer::singleShot(500, this, SLOT(_setRootPath()));
    } else {
        _setRootPath();
    }
}

void Sidebar::setActiveFile(QString path)
{
    if (!fileModel) {
        return;
    }
    QModelIndex index = ((QFileSystemModel*)model())->index(path);
    if (index.isValid()) {
        setSelection(visualRect(index), QItemSelectionModel::ClearAndSelect);
    }
}

void Sidebar::dataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    MainWindow* mw = MainWindow::instance();
    if (!mw->settings.isObject()) {
        return;
    }

    Json::Value file_exclude_patterns = mw->settings["file_exclude_patterns"];
    int rows = fileModel->rowCount(topLeft);
    for (int i = 0; i < rows; i++) {
        QModelIndex rowIndex = fileModel->index(i, 0, topLeft);
        QVariant rowData = fileModel->data(rowIndex);
        QString fileName = rowData.toString();
        QFileInfo info(fileName);

        // todo convert to QRegExp
        QString _suffix = "*." + info.suffix();

        if (file_exclude_patterns.isArray()) {
            for (int j = 0; j < file_exclude_patterns.size(); j++) {
                QString pat = file_exclude_patterns[j].asString().c_str();
                if (_suffix == pat) {
                    setRowHidden(i, topLeft, true);
                    break;
                }
            }
        }
    }

    animateShow();
}

void Sidebar::mouseDoubleClickEvent(QMouseEvent* event)
{
    // QTreeView::mouseDoubleClickEvent(event);
}

void Sidebar::mousePressEvent(QMouseEvent* event)
{
    QTreeView::mousePressEvent(event);
    
    if (updateTimer.isActive()) {
        return;
    }

    updateTimer.start(150);

    QModelIndex index = currentIndex();
    if (index.isValid()) {
        MainWindow *mw = MainWindow::instance();
        QString fileName = fileModel->filePath(index);

        QString btn = event->button() == Qt::RightButton ? "right" : "left";
        mw->emitEvent("sidebarItemClicked", "{\"path\": \"" + fileName + "\", \"button\": \"" + btn + "\"}");
        if (btn == "right") {
            return;
        }
        
        if (QFileInfo(fileName).isDir()) {
            isExpanded(index) ? collapse(index) : expand(index);
        } else {
            mw->openFile(fileName);
        }
    }
}
void Sidebar::_setRootPath()
{
    // qDebug() << "root:" << rootPath;
    fileModel->setRootPath(rootPath);

    QModelIndex idx = fileModel->index(fileModel->rootPath());
    setRootIndex(idx);
}

void Sidebar::animateShow()
{
    MainWindow *mw = MainWindow::instance();
    width = 0;
    targetWidth = 250;
    
    mw->horizontalSplitter()->setSizes({0, mw->width()});
    animateTimer.start(50);
    show();
}

void Sidebar::animateHide()
{
    hide();
}

void Sidebar::onAnimate()
{
    MainWindow *mw = MainWindow::instance();
    float d = targetWidth - width;
    width += (d * 0.6);
    if (d < 4) {
        width = targetWidth;
        animateTimer.stop();
    }
    
    MainWindow::instance()->horizontalSplitter()->setSizes({width, mw->width()});
}