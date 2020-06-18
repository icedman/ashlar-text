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
            bool expanded = ((QTreeView*)parent())->isExpanded(index);;
            return icon_for_folder(mainWindow->icons, fileName, expanded, mainWindow->extensions);
        }
    }

    return QFileSystemModel::data(index, role);
}

Sidebar::Sidebar(QWidget* parent)
    : QTreeView(parent)
    , fileModel(0)
{
    setHeaderHidden(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(singleClick()));
    connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(expandItem(const QModelIndex &)));
}

void Sidebar::setRootPath(QString path)
{
    if (fileModel) {
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

void Sidebar::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    // QModelIndexList i = selected.indexes();
    // if (i.size()) {
    //     QString filePath = fileModel->filePath(i[0]);
    //     mainWindow->openFile(filePath);
    // }
}

void Sidebar::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    // open
    timer.stop();

    QTreeView::mouseDoubleClickEvent(event);
}

void Sidebar::mousePressEvent(QMouseEvent* event)
{
    Q_UNUSED(event);
    timer.start(50);

    QTreeView::mousePressEvent(event);
}

void Sidebar::singleClick()
{
    QModelIndex index = currentIndex();
    if (index.isValid()) {
        QString filePath = fileModel->filePath(index);
        mainWindow->openFile(filePath);
    }

    timer.stop();
}

void Sidebar::expandItem(const QModelIndex &index)
{
    isExpanded(index) ? collapse(index) : expand(index);
}
