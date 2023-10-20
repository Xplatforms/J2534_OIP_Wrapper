#include "defs.h"
#include "exj2534emulator.h"
#include <QTimer>
#include <QCborValue>
#include <QCborMap>
#include <QCborArray>




ExJ2534Emulator::ExJ2534Emulator(QObject * parent):QAbstractListModel(parent)
{
    this->m_roles[ID] = "ID";
    this->m_roles[Timestamp] = "Timestamp";
    this->m_roles[Message] = "Message";

    this->p_read_msgs = new ExJ2534ReadMsgsList(this);
    this->p_write_msgs = new ExJ2534WriteMsgsList(this);

}

ExJ2534Emulator * ExJ2534Emulator::getInstance()
{
    if(ExJ2534Emulator::s_ExJ2534Emulator_singletonInstance == nullptr)ExJ2534Emulator::s_ExJ2534Emulator_singletonInstance = new ExJ2534Emulator(nullptr);

    return ExJ2534Emulator::s_ExJ2534Emulator_singletonInstance;
}

ExJ2534Emulator * ExJ2534Emulator::create(QQmlEngine *, QJSEngine *engine)
{
    auto instance = ExJ2534Emulator::getInstance();
    // The instance has to exist before it is used. We cannot replace it.
    Q_ASSERT(instance);

    // The engine has to have the same thread affinity as the singleton.
    Q_ASSERT(engine->thread() == instance->thread());

    // There can only be one engine accessing the singleton.
    if (s_engine)
        Q_ASSERT(engine == s_engine);
    else
        s_engine = engine;

    // Explicitly specify C++ ownership so that the engine doesn't delete
    // the instance.
    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
    return instance;
}


void ExJ2534Emulator::processPipeMessage(PipeMessage msg)
{
    if(msg.timestamp == 0)msg.timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();
    auto obj = new TimestampedObject{msg.timestamp, msg};
    QTimer::singleShot(0,[this, obj]{this->addElement(obj);});

    if(msg.data_type == 2)
    {
        auto cbor = QCborValue::fromCbor(msg.data).toMap();
        switch(cbor.value(PipeMessage::KEY_FuncName).toInteger(-1))
        {
        case PipeMessage::KEY_PassThruReadMsgs:
            this->p_read_msgs->processPipeMessage(msg);
            break;
        case PipeMessage::KEY_PassThruWriteMsgs:
            this->p_write_msgs->processPipeMessage(msg);
            break;
        }
    }
}

void ExJ2534Emulator::addElement(TimestampedObject * obj)
{
    this->beginInsertRows(QModelIndex(), this->rowCount(), this->rowCount());
    this->p_objs.append(obj);
    this->endInsertRows();
}

void ExJ2534Emulator::removeElement(TimestampedObject * obj)
{
    auto idx = this->p_objs.indexOf(obj);
    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->p_objs.removeOne(obj);
    this->endRemoveRows();
}

void ExJ2534Emulator::removeElementByIndex(qint32 idx)
{
    auto obj = this->p_objs.at(idx);
    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->p_objs.removeOne(obj);
    this->endRemoveRows();
}


QVariant ExJ2534Emulator::data(const QModelIndex & index, int role) const
{
    if (index.row() < 0 || index.row() > this->p_objs.size())return QVariant();

    auto obj = this->p_objs[index.row()];

    switch (role) {
    case ID:
        return obj->msg.id;
    case Timestamp:
        return obj->timestamp;
    case Message:
        return obj->msg.message;
    }

    return QVariant();
}
