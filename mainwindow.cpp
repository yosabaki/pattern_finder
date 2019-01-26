#include "custommodel.h"
#include "mainwindow.h"
#include "searcher.h"
#include "ui_mainwindow.h"

#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFileSystemModel>
#include <QMessageBox>
#include <vector>
#include <QThread>
#include <QDesktopServices>
#include <QDesktopServices>
#include <QScrollBar>
#include <QFutureWatcher>
#include <QPainter>
#include <QtConcurrent/QtConcurrent>

mainWindow::mainWindow(QWidget *parent):  QMainWindow(parent), searcher(nullptr), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    dirModel = new CustomModel(this);

    dirModel->setRootPath(QDir::rootPath());
    dirModel->setFilter(QDir::NoDotAndDotDot | QDir::AllEntries);

    ui->treeView->setModel(dirModel);

    ui->cancelSearchButton->hide();
    ui->cancelWatchButton->hide();
    ui->showPatternLines->hide();


//    ui->treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
//    ui->treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    ui->treeView->hideColumn(1);
    ui->treeView->hideColumn(2);
    ui->treeView->hideColumn(3);
    ui->treeView->setAutoScroll(true);
    ui->treeView->setMaximumWidth(QApplication::desktop()->screenGeometry().width() / 4);

    ui->treeView->scrollTo(dirModel->index(QDir::homePath()));

    ui->listWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    QCommonStyle style;
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));

    ui->progressBar->hide();

    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &mainWindow::show_about_dialog);
    connect(ui->listWidget, &QListWidget::customContextMenuRequested, this, &mainWindow::openItemMenu);
    connect(ui->watchButton, &QPushButton::clicked, this, &mainWindow::watch);
    connect(ui->searchButton, &QPushButton::clicked, this, &mainWindow::search);
    connect(ui->cancelSearchButton, &QPushButton::clicked, this, &mainWindow::cancelSearch);
    connect(ui->cancelWatchButton, &QPushButton::clicked, this, &mainWindow::cancelWatch);
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, &mainWindow::showFile);
    curr_dir = QDir::homePath();

    setWindowTitle(QString("Directory Content - %1").arg(curr_dir));
}

mainWindow::~mainWindow() {
    if (searcher!=nullptr) {
        cancelWatch();
        cancelSearch();
        searchWatcher.waitForFinished();
        watchWatcher.waitForFinished();
    }
    delete dirModel;
}

void mainWindow::modelToVector(QModelIndex const &index, QVector<QString> &files) {
    QString path = dirModel->filePath(index);
    if (dirModel->isChecked(index) == Qt::Checked) {
        files.push_back(path);
    } else if (dirModel->isChecked(index) == Qt::PartiallyChecked) {
        for (int i = 0; i < dirModel->rowCount(index); i++) {
            modelToVector(dirModel->index(i, 0, index), files);
        }
    }
}

void mainWindow::watch() {
    ui->listWidget->clear();
    blockWatch();
    QVector<QString> files;
    modelToVector(dirModel->index(dirModel->rootPath(),0), files);
    searcher.reset(new Searcher(nullptr, files));
    connect(searcher.get(), &Searcher::requestReindex, this, &mainWindow::requestReindex);
    connect(searcher.get(), &Searcher::progressBarChanged, this, &mainWindow::setProgressBar);
    connect(searcher.get(), &Searcher::finished, this, &mainWindow::unblockWatch);
    watchWatcher.setFuture(QtConcurrent::run(searcher.get(), &Searcher::process));
}

void mainWindow::requestReindex() {
    QMessageBox::StandardButton alert;
    alert = QMessageBox::question(this,"Reindex files?","Some files have been changed.", QMessageBox::Yes | QMessageBox::No);
    if (alert == QMessageBox::No) {
        return;
    } else {
        if (searcher!= nullptr) {
            searcher->cancel();
            searchWatcher.waitForFinished();
            watchWatcher.waitForFinished();
        }
        watch();
    }
}

void mainWindow::blockWatch() {
    setProgressBar(0);
    ui->showPatternLines->hide();
    ui->searchButton->setDisabled(true);
    ui->cancelWatchButton->show();
    ui->watchButton->hide();
    ui->progressBar->show();
    dirModel->setLock(true);
    dirModel->marked.clear();
}

void mainWindow::unblockWatch() {
    ui->searchButton->setEnabled(true);
    ui->cancelWatchButton->hide();
    ui->watchButton->show();
    ui->progressBar->hide();
    dirModel->setLock(false);
    if (searcher->success) {
        for (auto file:searcher->files) {
            dirModel->setMarked(dirModel->index(file),true);
        }
    }
    disconnect(searcher.get(), &Searcher::progressBarChanged, this, &mainWindow::setProgressBar);
    disconnect(searcher.get(), &Searcher::finished, this, &mainWindow::unblockWatch);
}

void mainWindow::search() {
    ui->listWidget->clear();
    patternString = ui->patternEdit->text();
    if (patternString.size() < 3 || patternString.size() > 1000) {
        QMessageBox::StandardButton alert;
        QString message = (patternString.size()<3?"Pattern string must be bigger than 2 symbols": "Pattern string is too long. it mast be shorter than 1000 symbols");
        alert = QMessageBox::information(this,"can't search", message ,QMessageBox::Ok);
        if (alert == QMessageBox::Ok) {
            return;
        }
    }
    blockSearch();
    QVector<QString> files;
    modelToVector(dirModel->index(dirModel->rootPath(),0), files);
    searcher->setPattern(patternString);
    connect(searcher.get(), &Searcher::progressBarChanged, this, &mainWindow::setProgressBar);
    connect(searcher.get(), &Searcher::finished, this, &mainWindow::unblockSearch);
    connect(searcher.get(), &Searcher::itemAdded, this, &mainWindow::addItem);
    searchWatcher.setFuture(QtConcurrent::run(searcher.get(), &Searcher::search));
}

void mainWindow::blockSearch() {
    ui->showPatternLines->hide();
    setProgressBar(0);
    ui->watchButton->setDisabled(true);
    ui->cancelSearchButton->show();
    ui->searchButton->hide();
    ui->progressBar->show();
    dirModel->setLock(true);
}

void mainWindow::unblockSearch() {
    ui->cancelSearchButton->hide();
    ui->searchButton->show();
    ui->watchButton->setEnabled(true);
    ui->cancelSearchButton->hide();
    ui->progressBar->hide();
    dirModel->setLock(false);
    disconnect(searcher.get(), &Searcher::progressBarChanged, this, &mainWindow::setProgressBar);
    disconnect(searcher.get(), &Searcher::finished, this, &mainWindow::unblockSearch);
    disconnect(searcher.get(), &Searcher::itemAdded, this, &mainWindow::addItem);
}

void mainWindow::addItem(QString filePath) {
    ui->listWidget->addItem(filePath);
}

void mainWindow::cancelSearch() {
    searcher->cancel();
}

void mainWindow::cancelWatch() {
    searcher->cancel();
}

void mainWindow::showFile(QListWidgetItem *item) {
    ui->showPatternLines->clear();
    ui->showPatternLines->show();
    QFile file(item->text());
    if (file.open(QFile::ReadOnly)) {
        int count = 0;
        static const int MAX_SHOW_LINE_SIZE = 1000;
        while(!file.atEnd()) {
            count++;
            QString line = file.readLine();
            int index = line.indexOf(patternString);
            if (index >= 0) {
                ui->showPatternLines->append(QString::number(count)+':'+(line.size()<MAX_SHOW_LINE_SIZE ? line : (index==0?"":"...")+line.mid(std::max(0,index - 10),patternString.size()+20)+"..."));
            }
        }
        QTextCursor cursor(ui->showPatternLines->document());
        QTextCharFormat fmt;
        fmt.setBackground(QColor(200,200,255));
        QString lines = ui->showPatternLines->toPlainText();
        int pos = lines.indexOf(':');
        pos = lines.indexOf(patternString, pos + 1);
        while (pos >= 0) {
            cursor.setPosition(pos, QTextCursor::MoveAnchor);
            cursor.setPosition(pos + patternString.length(), QTextCursor::KeepAnchor);
            cursor.setCharFormat(fmt);
            pos = lines.indexOf(patternString, pos + 1);
        }
    } else {
        ui->showPatternLines->append(file.errorString());
        return;
    }
}


void mainWindow::openItemMenu(const QPoint &pos) {
    clickedItem = ui->listWidget->itemAt(pos);

    QAction *openItemAction = new QAction(tr("Open file"),this);
    QAction *showFolderAction = new QAction(tr("Show containing folder"),this);
    connect(openItemAction, &QAction::triggered, this, &mainWindow::openItem);
    connect(showFolderAction, &QAction::triggered, this, &mainWindow::showFolder);
    QMenu menu(this);
    menu.addAction(openItemAction);
    menu.addAction(showFolderAction);
    menu.exec( ui->listWidget->mapToGlobal(pos) );
    clearClickedItem();
}

void mainWindow::clearClickedItem() {
    clickedItem = nullptr;
}

void mainWindow::openItem() {
    if (clickedItem == nullptr) return;
    QString path = clickedItem->text();
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

void mainWindow::showFolder() {
    if (clickedItem == nullptr) return;
    QDir dir(clickedItem->text());
    dir.cdUp();
    QDesktopServices::openUrl(QUrl::fromLocalFile(dir.path()));
}

QString getSizeOfFileString(size_t size) {
    std::vector<std::string> suffixes = {"B", "KB", "MB", "GB", "TB"};
    double d_size = size;
    size_t index = 0;
    while (d_size >= 512 && index < suffixes.size()) {
        d_size /= 1024.0;
        index++;
    }
    return QString::number(d_size, 'f', (index > 0)).append(suffixes[index].c_str());
}

void mainWindow::setProgressBar(int progress) {
    ui->progressBar->setValue(progress);
}

void mainWindow::addScannedFiles(QVector<QList<QString> > files) {
    size_t n = files.size();
    QFileIconProvider iconProvider;
    for (size_t i = 0; i < n; i++) {

        QColor color(rand() % 256, rand() % 256, rand() % 256);
        QColor textColor("black");
        if (color.black() > 100) {
            textColor = QColor("white");
        }

    }
}

void mainWindow::show_about_dialog() {
    QMessageBox::aboutQt(this);
}
