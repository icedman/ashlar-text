#include <QDebug>
#include <QDir>
#include <QSplitter>

#include <iostream>

#include "Cubic.h"
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
        MainWindow* mw = MainWindow::instance();
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
    , clickTimer(this)
{
    setHeaderHidden(true);
    setAnimated(true);
    hide();

    setMinimumSize(0, 0);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    clickTimer.setSingleShot(true);

    connect(&animateTimer, SIGNAL(timeout()), this, SLOT(onAnimate()));
}

void Sidebar::setRootPath(QString path, bool deferred, bool show)
{
    if (fileModel || path.isEmpty()) {
        return;
    }

    showOnLoad = show;
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
   
    int rows = fileModel->rowCount(topLeft);
    for (int i = 0; i < rows; i++) {
        QModelIndex rowIndex = fileModel->index(i, 0, topLeft);
        QVariant rowData = fileModel->data(rowIndex);
        QString fileName = rowData.toString();
        QFileInfo info(fileName);

        // todo convert to QRegExp
        QString _suffix = "*." + info.suffix();

        for(auto pat : excludeFiles) {
            if (_suffix == pat) {
                setRowHidden(i, topLeft, true);
                break;
            }
        }
    }

    if (!isVisible() && showOnLoad) {
        animateShow();
    }
}

void Sidebar::mouseDoubleClickEvent(QMouseEvent* event)
{
    // QTreeView::mouseDoubleClickEvent(event);
}

void Sidebar::mousePressEvent(QMouseEvent* event)
{
    QTreeView::mousePressEvent(event);

    if (clickTimer.isActive()) {
        return;
    }
    clickTimer.start(150);

    QModelIndex index = currentIndex();
    if (index.isValid()) {
        MainWindow* mw = MainWindow::instance();
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
    MainWindow* mw = MainWindow::instance();
    
    Json::Value file_exclude_patterns = mw->settings["file_exclude_patterns"];
    if (!excludeFiles.length() && file_exclude_patterns.isArray() && file_exclude_patterns.size()) {
        for (int j = 0; j < file_exclude_patterns.size(); j++) {
            QString pat = file_exclude_patterns[j].asString().c_str();
            excludeFiles << pat;
        }
    }
    
    Json::Value folder_exclude_patterns = mw->settings["folder_exclude_patterns"];
    if (!excludeFolders.length() && folder_exclude_patterns.isArray() && folder_exclude_patterns.size()) {
        for (int j = 0; j < folder_exclude_patterns.size(); j++) {
            QString pat = folder_exclude_patterns[j].asString().c_str();
            excludeFolders << pat;
        }
    }
    
    // qDebug() << "root:" << rootPath;
    fileModel->setRootPath(rootPath);

    QModelIndex idx = fileModel->index(fileModel->rootPath());
    setRootIndex(idx);
}

void Sidebar::animateShow()
{
    if (isVisible()) {
        return;
    }

    MainWindow* mw = MainWindow::instance();
    animTime = -250;
    width = 0;
    targetWidth = 250;

    mw->horizontalSplitter()->setSizes({ 0, mw->width() });
    animateTimer.start(25);
}

void Sidebar::animateHide()
{
    hide();
}

void Sidebar::onAnimate()
{
    MainWindow* mw = MainWindow::instance();

    const int duration = 250;
    animTime += animateTimer.interval();
    if (animTime < 0) {
        return;
    }
    width = Cubic::easeOut(animTime, 0, targetWidth, duration);
    if (animTime >= duration) {
        width = targetWidth;
        animateTimer.stop();
    }

    setVisible(width > 0);
    mw->horizontalSplitter()->setSizes({ width, mw->width() });
}

void Sidebar::fetchFiles(QFileSystemModel *m, QModelIndex index, QStringList &res) {
    for(int i=0; i<m->rowCount(index); i++) {
        QModelIndex childIndex = m->index(i, 0, index);
        
        if (m->canFetchMore(childIndex)) {
            m->fetchMore(childIndex);
        }

        if (m->isDir(childIndex)) {
            fetchFiles(m, childIndex, res);
        } else {
            
            bool skip = false;
            
            QString fileName = m->filePath(childIndex);
            QFileInfo info(fileName);

            // todo convert to QRegExp
            QString _suffix = "*." + info.suffix();
            
            for(auto pat : excludeFiles) {
                if (_suffix == pat) {
                    skip = true;
                    break;
                }
            }
            
            if (!skip) {
                res << fileName;
            }
        }
    }
}

QStringList Sidebar::allFiles()
{
    QStringList files;
    FileSystemModel *fs = fileModel;
    fetchFiles(fs, fs->index(rootPath), files);
    return files;
}
