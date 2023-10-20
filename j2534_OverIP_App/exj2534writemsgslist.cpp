#include "exj2534writemsgslist.h"
#include <QDateTime>

ExJ2534WriteMsgsList::ExJ2534WriteMsgsList(QObject *parent)
    : QAbstractListModel{parent}
{

}

QVariant ExJ2534WriteMsgsList::data(const QModelIndex & index, int role) const
{
    return QVariant();
}

void ExJ2534WriteMsgsList::processPipeMessage(PipeMessage msg)
{
    if(msg.timestamp == 0)msg.timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();
    auto obj = new TimestampedObject{0, msg};
    this->addElement(obj);
}

void ExJ2534WriteMsgsList::addElement(TimestampedObject * obj)
{
    this->beginInsertRows(QModelIndex(), this->rowCount(), this->rowCount());
    this->p_objs.append(obj);
    this->endInsertRows();
}

void ExJ2534WriteMsgsList::removeElement(TimestampedObject * obj)
{
    auto idx = this->p_objs.indexOf(obj);
    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->p_objs.removeOne(obj);
    this->endRemoveRows();
}

void ExJ2534WriteMsgsList::removeElementByIndex(qint32 idx)
{
    auto obj = this->p_objs.at(idx);
    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->p_objs.removeOne(obj);
    this->endRemoveRows();
}
