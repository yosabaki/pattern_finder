#include "custommodel.h"

QVariant CustomModel::data(const QModelIndex& index, int role) const {
    if (role == Qt::BackgroundColorRole) {
        return (isMarked(index) ? QColor(0,0,0) : QColor(255,255,255));
    }
    if (role == Qt::CheckStateRole) {
        return isChecked(index);
    }
    return QFileSystemModel::data(index, role);
}

Qt::ItemFlags CustomModel::flags(const QModelIndex& index) const {
    return QFileSystemModel::flags(index) | Qt::ItemIsUserCheckable;
}

void CustomModel::setMarked(QModelIndex const& index, bool mark) {
    marked[filePath(index)] = mark;
}

bool CustomModel::isMarked(const QModelIndex &index) const {
    auto it = marked.find(filePath(index));
    return (it == marked.end()? false: *it);
}

void CustomModel::setLock(bool _lock) {
    this->lock = _lock;
}

int CustomModel::isChecked(const QModelIndex &index) const {
    int size = checkedNumber[filePath(index)];
    if (size == 0) {
        return Qt::Unchecked;
    } else if (size >= rowCount(index) * 2) {
        return Qt::Checked;
    }
    return Qt::PartiallyChecked;
}

void CustomModel::checkLoadedDirectory(const QString &path) {
    QModelIndex index = this->index(path);
    if (checkedNumber[path] > 0) {
        setChildrenCheck(index, true);
    }
}

void CustomModel::setParentCheck(QModelIndex index, int prevValue, int currValue) {
    QString path = filePath(index.parent());
    int nextPrev = checkedNumber[path];
    if (currValue == prevValue) return;
    if (currValue > prevValue) {
        if (prevValue == 0) {
            checkedNumber[path] ++;
        }
        if (isChecked(index) == Qt::Checked){
            checkedNumber[path] ++;
        }
    } else {
        if (prevValue >= rowCount(index) * 2 && prevValue > 0) {
            checkedNumber[path] --;
        }
        if (isChecked(index) == Qt::Unchecked){
            checkedNumber[path] --;
        }
    }
    emit dataChanged(index.parent(), index.parent());
    if (filePath(index) != "/") {
        setParentCheck(index.parent(), nextPrev, checkedNumber[filePath(index.parent())]);
    }
}

void CustomModel::setChildrenCheck(QModelIndex index, bool check) {
    QString path = filePath(index);
    if (check) {
        checkedNumber[path] = (rowCount(index) == 0? 1: rowCount(index) * 2);
    } else {
        checkedNumber[path] = 0;
    }
    emit dataChanged(index,index);
    for (int i = 0; i < rowCount(index); i++) {
        setChildrenCheck(this->index(i, 0, index), check);
    }
}

bool CustomModel::setData(const QModelIndex& index, const QVariant& value, int role) {
    if (lock) return false;
    QString path = filePath(index);
    if (role == Qt::CheckStateRole) {
        bool tmp;
        int prevValue = checkedNumber[path];
        if (prevValue == 0) {
            tmp = true;
            checkedNumber[path] = (rowCount(index) == 0? 1 : rowCount(index) * 2);
        } else {
            tmp = false;
            checkedNumber[path] = 0;
        }

        setParentCheck(index, prevValue, checkedNumber[path]);

        setChildrenCheck(index, tmp);

        emit dataChanged(index, index);
        return true;
    }
    return QFileSystemModel::setData(index, value, role);
}
