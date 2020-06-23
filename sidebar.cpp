#include <QDebug>
#include <QDir>

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
        QFileInfo info = fileInfo(index);
        QString fileName = info.fileName();
        if (info.isFile()) {
            QString suffix = info.suffix();
            return icon_for_file(mainWindow->icons, fileName, suffix, mainWindow->extensions);
        } else {
            bool expanded = ((QTreeView*)parent())->isExpanded(index);
            ;
            return icon_for_folder(mainWindow->icons, fileName, expanded, mainWindow->extensions);
        }
    }

    return QFileSystemModel::data(index, role);
}

Sidebar::Sidebar(QWidget* parent)
    : QTreeView(parent)
    , fileModel(0)
    , updateTimer(this)
{
    setHeaderHidden(true);
    setAnimated(true);
    hide();
    
    setMinimumSize(250, 0);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    updateTimer.setSingleShot(true);
}

void Sidebar::setRootPath(QString path)
{
    if (fileModel || path.isEmpty()) {
        return;
    }

    MainWindow* main = MainWindow::instance();

    fileModel = new FileSystemModel(this);
    fileModel->mainWindow = main;
    fileModel->setRootPath(path);

    setModel(fileModel);

    for (int i = 1; i < fileModel->columnCount(); i++) {
        hideColumn(i);
    }

    QModelIndex idx = fileModel->index(fileModel->rootPath());
    setRootIndex(idx);

    show();
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
    MainWindow* main = mainWindow;
    if (!main->settings.isObject()) {
        return;
    }

    Json::Value file_exclude_patterns = main->settings["file_exclude_patterns"];
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
}

void Sidebar::mouseDoubleClickEvent(QMouseEvent* event)
{
    // QTreeView::mouseDoubleClickEvent(event);
}

void Sidebar::mousePressEvent(QMouseEvent* event)
{
    QTreeView::mousePressEvent(event);
    
    singleClick();
}

void Sidebar::singleClick()
{
    if (updateTimer.isActive()) {
        return;
    }
    updateTimer.start(150);
    
    QModelIndex index = currentIndex();
    if (index.isValid()) {
        QString fileName = fileModel->filePath(index);        
        
        if (QFileInfo(fileName).isDir()) {
            isExpanded(index) ? collapse(index) : expand(index);
        } else {
            mainWindow->openFile(fileName);
        }
    }
}
