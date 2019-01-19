#include "searcher.h"

#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <string>
Searcher::Searcher(QObject *parent, QVector<QString> const& files) : QObject(parent), files(files), isCanceled(false) {
    connect(&fileWatcher, &QFileSystemWatcher::fileChanged, this, &Searcher::reindex);
}

Searcher::Searcher(QObject *parent) : QObject(parent) {

}

void Searcher::reindex(QString const& filePath) {
    emit requestReindex();
}

void Searcher::cancel() {
    isCanceled = true;
}

void Searcher::fillFileIndecies() {
    for (auto &dir: files) {
        if (QThread::currentThread()->isInterruptionRequested()) break;
        QDirIterator it(dir, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            if (QThread::currentThread()->isInterruptionRequested()) break;
            QFileInfo file_info = it.fileInfo();
            if (file_info.isFile()) {
                fileIndecies.push_back(new FileIndex(file_info.filePath()));
            }
            it.next();
        }
    }
}

Searcher::~Searcher() {
    for (int i = 0;i < fileIndecies.size();i++) {
        delete fileIndecies[i];
    }
}

bool Searcher::canConvertedToUtf8(const QString &string) {
    if (string.lastIndexOf('\0')>=0) {
        return false;
    }
    return true;
}

void addTrgsToIndex(QString const& string, FileIndex *index) {
    if (string.size() < 3) return;
    uint32_t trg = 0 | (static_cast<uint8_t>(string[0].unicode()) << 8) | static_cast<uint8_t>(string[1].unicode());
    for (int i = 2; i < string.length(); i++) {
        trg <<= 8;
        trg |= string[i].unicode();
        index->insertTrg(trg & 0xFFFFFF);
    }
}

QVector<uint32_t> splitStringToTrgs(QString &string) {
    QVector<uint32_t> trgs;
    if (string.size() < 3) return trgs;
    uint32_t trg = 0 | (static_cast<uint8_t>(string[0].unicode()) << 8) | static_cast<uint8_t>(string[1].unicode());
    for (int i = 2; i < string.length(); i++) {
        trg <<= 8;
        trg |= string[i].unicode();
        trgs.push_back(trg & 0xFFFFFF);
    }
    return trgs;
}

void Searcher::indexFile(FileIndex *index) {
    QFile file(index->getFilePath());
    if (file.size() > MAX_READABLE_FILE_SIZE) {
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    QTextStream in(&file);
    while (!in.atEnd()) {

        QString data = in.readLine();
        if (data.size()>READ_BUFFER_SIZE) {
            index->clearTrgs();
            return;
        }
        if (!canConvertedToUtf8(data)) {
            index->clearTrgs();
            return;
        }
        addTrgsToIndex(data, index);
        if (index->size() > MAX_TRG_SIZE) {
            index->clearTrgs();
            return;
        }
    }
}

void Searcher::setPattern(const QString &string) {
    pattern = string;
}

bool containsTrg(QString &pattern, FileIndex *index) {
    QVector<uint32_t> pattern_trgs = splitStringToTrgs(pattern);
    for (uint32_t const& i : pattern_trgs) {
        if (!index->containsTrg(i)) {
            return false;
        }
    }
    return true;
}

void Searcher::search() {
    for (auto index : fileIndecies){
        if (isCanceled) {
            break;
        }
        if (containsTrg(pattern, index)) {
            QFile file(index->getFilePath());
            if (file.open(QFile::ReadOnly)) {
                while (!file.atEnd()) {
                    if (isCanceled) {
                        break;
                    }
                    QString line = file.readLine();
                    if (line.indexOf(pattern) >= 0) {
                        emit itemAdded(index->getFilePath());
                        break;
                    }
                }
            } else {
                qDebug()<<"LOL";
            }
        }
    }
    isCanceled = false;
    emit finished();
}

void Searcher::process() {
    success = false;
    fillFileIndecies();
    for (int i = 0; i < fileIndecies.size(); ++i) {
        if (isCanceled) {
            fileIndecies.clear();
            break;
        }
        emit progressBarChanged((i * 100) / fileIndecies.size());
        fileWatcher.addPath(fileIndecies[i]->getFilePath());
        indexFile(fileIndecies[i]);
    }
    qDebug()<< fileIndecies.size()<<'\n';
    if (!isCanceled) {
        success = true;
    }
    isCanceled = false;
    emit finished();
}

