#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "custommodel.h"
#include "fileindex.h"
#include "searcher.h"

#include <QFileSystemModel>
#include <QMainWindow>
#include <QPoint>
#include <QListWidget>
#include <memory>
#include <QFutureWatcher>

namespace Ui {
class MainWindow;
}

class mainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit mainWindow(QWidget *parent = 0);
    ~mainWindow();

private slots:
    void requestReindex();
    void show_about_dialog();
    void openItem();
    void openItemMenu(const QPoint &pos);
    void showFolder();
    void clearClickedItem();
    void watch();
    void search();
    void cancelWatch();
    void cancelSearch();
    void showFile(QListWidgetItem *item);
    void blockWatch();
    void unblockWatch();
    void blockSearch();
    void unblockSearch();
    void modelToVector(QModelIndex const &index, QVector<QString> &files);
public slots:
    void addScannedFiles(QVector<QList<QString>> files);
    void setProgressBar(int progress);
    void addItem(QString filePath);
private:
    std::unique_ptr<Searcher> searcher;
    QFutureWatcher<void> searchWatcher, watchWatcher;
    QString patternString;
    QListWidgetItem *clickedItem;
    CustomModel *dirModel;
    QString curr_dir;
    std::unique_ptr<Ui::MainWindow> ui;
};

#endif // MAINWINDOW_H
