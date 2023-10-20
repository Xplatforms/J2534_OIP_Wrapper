#ifndef EXJ2534EMULATOR_H
#define EXJ2534EMULATOR_H

#include <QObject>
#include <QAbstractListModel>
#include <QQmlEngine>
#include <QJSEngine>

#include "types.h"
#include "exj2534readmsgslist.h"
#include "exj2534writemsgslist.h"



class ExJ2534Emulator : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ExJ2534Emulator)
    QML_SINGLETON

public:
    enum ElemRoles
    {
        ID = Qt::UserRole + 1,
        Timestamp,
        Message
    };

    static ExJ2534Emulator * getInstance();
    static ExJ2534Emulator *create(QQmlEngine *, QJSEngine *engine);

    int rowCount(const QModelIndex & parent = QModelIndex()) const {return this->p_objs.count();}
    QVariant data(const QModelIndex & index, int role = ExJ2534Emulator::ID) const;
    QHash<int, QByteArray> roleNames() const {return this->m_roles;}

public slots:
    void processPipeMessage(PipeMessage msg);
    void addElement(TimestampedObject * obj);
    void removeElement(TimestampedObject * obj);
    void removeElementByIndex(qint32 idx);

private:
    QHash<int, QByteArray>      m_roles;
    QList<TimestampedObject *>    p_objs;
    ExJ2534ReadMsgsList *       p_read_msgs;
    ExJ2534WriteMsgsList *      p_write_msgs;

private:
    ExJ2534Emulator(QObject * parent = nullptr);
    inline static ExJ2534Emulator *s_ExJ2534Emulator_singletonInstance = nullptr;
    inline static QJSEngine * s_engine = nullptr;

};

#endif // EXJ2534EMULATOR_H
