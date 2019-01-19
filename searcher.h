#ifndef SEARCHER_H
#define SEARCHER_H

#include "fileindex.h"

#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QObject>
#include <QVector>

class Searcher : public QObject
{
    Q_OBJECT
public:

    Searcher(QObject* object);

    Searcher(QObject* object, QVector<QString> const& files);

    ~Searcher();

    int getProgress();

    void indexFile(FileIndex *index);

    void setPattern(QString const& string);

    QVector<QString> files;

    bool success;
private:
    void fillFileIndecies();
    bool canConvertedToUtf8(QString const& string);
    QVector<uint32_t> splitIntoTrgs(QString const& string);
    const int MAX_READABLE_FILE_SIZE = 1 << 30,
              READ_BUFFER_SIZE = 1000,
              MAX_TRG_SIZE = 20000;
public slots:

    void process();

    void search();

    void cancel();

    void reindex(QString const &filePath);

signals:
    void progressBarChanged(int percent);

    void finished();

    void error(QString err);

    void itemAdded(QString path);

    void requestReindex();
private:
    QFileSystemWatcher fileWatcher;
    void indexFiles();

    bool isCanceled;

    int progressCount;
    QVector<FileIndex *> fileIndecies;
    QString pattern;
};


#endif // SEARCHER_H
