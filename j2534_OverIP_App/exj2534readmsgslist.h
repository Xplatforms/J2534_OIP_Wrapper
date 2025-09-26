#ifndef EXJ2534READMSGSLIST_H
#define EXJ2534READMSGSLIST_H

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

#include "types.h"
#include "PassThruStruct.h"

class ExJ2534ReadMsgsList : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ExJ2534ReadMsgsList)
    QML_UNCREATABLE("Should not be created directly in QML. Will have no functionality")

public:
    enum ElemRoles
    {
        ID = Qt::UserRole + 1,
        HasError,
        ProtocolID,
        RxStatus,
        TxFlags,
        Timestamp,
        DataSize,
        ExtraDataIndex,
        Data
    };



    explicit ExJ2534ReadMsgsList(QObject *parent = nullptr);

    int rowCount(const QModelIndex & parent = QModelIndex()) const {return this->p_objs.count();}
    QVariant data(const QModelIndex & index, int role = ExJ2534ReadMsgsList::ID) const;
    QHash<int, QByteArray> roleNames() const {return this->m_roles;}

public slots:
    void processPipeMessage(PipeMessage msg);
    void addElement(PASSTHRU_MSG * obj);
    void removeElement(PASSTHRU_MSG * obj);
    void removeElementByIndex(qint32 idx);

private:
    QHash<int, QByteArray>          m_roles;
    QList<PASSTHRU_MSG *>           p_objs;
};

#endif // EXJ2534READMSGSLIST_H
