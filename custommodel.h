#ifndef CUSTOMMODEL_H
#define CUSTOMMODEL_H

#include <QFileSystemModel>

class CustomModel : public QFileSystemModel {
public:
    CustomModel(QObject *parent) : QFileSystemModel(parent){
        connect(this, &CustomModel::directoryLoaded, this, &CustomModel::checkLoadedDirectory);
    }

    int isChecked(QModelIndex const& index) const;

    void setLock(bool lock);

    void setMarked(QModelIndex const& index, bool mark);

    bool isMarked(QModelIndex const& index) const;

    QMap<QString, bool> marked;

private slots:

    void checkLoadedDirectory(QString const &path);

private:
    QMap<QString, int> checkedNumber;


    QVariant data(const QModelIndex& index, int role) const;

    void setChildrenMark(QModelIndex const& index, bool mark);

    void setChildrenCheck(QModelIndex const& index, bool check);

    void setParentCheck(QModelIndex const& index, int prevValue, int currValue);

    Qt::ItemFlags flags(const QModelIndex& index) const;

    bool setData(const QModelIndex& index, const QVariant& value, int role);

    bool lock = false;
};

#endif // CUSTOMMODEL_H
