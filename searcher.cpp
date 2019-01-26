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
    if (string.lastIndexOf('\0') >= 0) {
        return false;
    }
    return true;
}

void addTrgsToIndex(QString const& string, FileIndex *index) {
    QVector<uint8_t> bytes;
    for (int i = 0; i < string.size(); i++) {
        ushort ch = string[i].unicode();
        if (ch > UINT8_MAX) {
            bytes.push_back(ch >> 8);
        }
        bytes.push_back(ch & 0xFF);
    }
    if (bytes.size() < 3) return;
    uint32_t trg = 0 | (bytes[0] << 8) | bytes[1];
    for (int i = 2; i < bytes.size(); i++) {
        trg <<= 8;
        trg |= bytes[i];
        index->insertTrg(trg & 0xFFFFFF);
    }
}

QVector<uint32_t> splitStringToTrgs(QString &string) {
    QVector<uint32_t> trgs;
    QVector<uint8_t> bytes;
    for (int i = 0; i < string.size(); i++) {
        ushort ch = string[i].unicode();
        if (ch > UINT8_MAX) {
            bytes.push_back(ch >> 8);
        }
        bytes.push_back(ch & 0xFF);
    }
    if (bytes.size() < 3) return trgs;
    uint32_t trg = 0 | (bytes[0] << 8) | bytes[1];
    for (int i = 2; i < bytes.size(); i++) {
        trg <<= 8;
        trg |= bytes[i];
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
    QString data;
    while (!in.atEnd()) {

        data += in.read(READ_BUFFER_SIZE);
        if (!canConvertedToUtf8(data)) {
            index->clearTrgs();
            return;
        }
        addTrgsToIndex(data, index);
        if (index->size() > MAX_TRG_SIZE) {
            index->clearTrgs();
            return;
        }
        data = data.mid(data.size()-4, 4);
    }
}

void Searcher::setPattern(const QString &string) {
    pattern = string;
}

bool containsTrg(QVector<uint32_t> pattern_trgs, FileIndex *index) {
    for (uint32_t const& i : pattern_trgs) {
        if (!index->containsTrg(i)) {
            return false;
        }
    }
    return true;
}

void Searcher::search() {
    for (int i = 0; i< fileIndecies.size(); ++i){
        if (isCanceled) {
            break;
        }
        emit progressBarChanged((i * 100) / fileIndecies.size());
        QVector<uint32_t> pattern_trgs = splitStringToTrgs(pattern);
        if (containsTrg(pattern_trgs, fileIndecies[i])) {
            QFile file(fileIndecies[i]->getFilePath());
            if (file.open(QFile::ReadOnly)) {
                QString line;
                while (!file.atEnd()) {
                    if (isCanceled) {
                        break;
                    }
                    line += file.read(1000);
                    if (line.indexOf(pattern) >= 0) {
                        emit itemAdded(fileIndecies[i]->getFilePath());
                        break;
                    }
                    line = line.mid(line.size()-pattern.size(), pattern.size());
                }
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

