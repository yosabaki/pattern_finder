#include "fileindex.h"

FileIndex::FileIndex(QString const& filePath) : QObject(), filePath(filePath) {

}

QString FileIndex::getFilePath() {
    return filePath;
}

void FileIndex::insertTrg(uint32_t trg) {
    trgs.insert(trg);
}

void FileIndex::clearTrgs() {
    trgs.clear();
}

bool FileIndex::containsTrg(uint32_t trg) {
    return trgs.contains(trg);
}

size_t FileIndex::size() {
    return trgs.size();
}
