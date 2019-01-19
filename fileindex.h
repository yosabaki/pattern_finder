#ifndef FILEINDEX_H
#define FILEINDEX_H

#include <QObject>
#include <QSet>

class FileIndex : public QObject {
    Q_OBJECT
public:
    FileIndex(QString const& filePath);
    ~FileIndex() = default;

    QString getFilePath();
    void insertTrg(uint32_t trg);
    void clearTrgs();
    bool containsTrg(uint32_t trg);
    size_t size();
private:
    QSet<uint32_t> trgs;
    QString filePath;
signals:

public slots:
};

#endif // FILEINDEX_H
