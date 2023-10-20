#ifndef EXJ2534WRITEMSGSLIST_H
#define EXJ2534WRITEMSGSLIST_H

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>
#include "types.h"

class ExJ2534WriteMsgsList : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ExJ2534WriteMsgsList)
    QML_UNCREATABLE("Should not be created directly in QML. Will have no functionality")

public:
    enum ElemRoles
    {
        ID = Qt::UserRole + 1,
    };

    explicit ExJ2534WriteMsgsList(QObject *parent = nullptr);

    int rowCount(const QModelIndex & parent = QModelIndex()) const {return this->p_objs.count();}
    QVariant data(const QModelIndex & index, int role = ExJ2534WriteMsgsList::ID) const;
    QHash<int, QByteArray> roleNames() const {return this->m_roles;}

public slots:
    void processPipeMessage(PipeMessage msg);
    void addElement(TimestampedObject * obj);
    void removeElement(TimestampedObject * obj);
    void removeElementByIndex(qint32 idx);

private:
    QHash<int, QByteArray>          m_roles;
    QList<TimestampedObject *>      p_objs;
};

#endif // EXJ2534WRITEMSGSLIST_H
