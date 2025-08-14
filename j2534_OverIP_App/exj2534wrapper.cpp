#include "exj2534wrapper.h"
#include "defs.h"
#include <QTimer>
#include <QCborValue>
#include <QCborMap>
#include <QCborArray>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QGuiApplication>
#include <QFile>


ExJ2534Wrapper::ExJ2534Wrapper(QObject * parent):QAbstractListModel(parent)
{
    this->m_roles[ID] = "ID";
    this->m_roles[Timestamp] = "Timestamp";
    this->m_roles[Message] = "Message";
    this->m_roles[HasError] = "HasError";

    this->p_read_msgs = new ExJ2534ReadMsgsList(this);
    this->p_write_msgs = new ExJ2534WriteMsgsList(this);

    this->p_loaded_funcs = nullptr;
    this->p_loaded_dll_funcs = new QStringListModel(this);
    this->p_unloaded_dll_funcs = new QStringListModel(this);
}

ExJ2534Wrapper * ExJ2534Wrapper::getInstance()
{
    if(ExJ2534Wrapper::s_ExJ2534Wrapper_singletonInstance == nullptr)ExJ2534Wrapper::s_ExJ2534Wrapper_singletonInstance = new ExJ2534Wrapper(nullptr);

    return ExJ2534Wrapper::s_ExJ2534Wrapper_singletonInstance;
}

ExJ2534Wrapper * ExJ2534Wrapper::create(QQmlEngine *, QJSEngine *engine)
{
    auto instance = ExJ2534Wrapper::getInstance();
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

QStringListModel * ExJ2534Wrapper::loaded_dll_funcs()
{
    this->p_loaded_dll_funcs->setStringList(this->getDllFuncs());
    return this->p_loaded_dll_funcs;
}

QStringListModel * ExJ2534Wrapper::unloaded_dll_funcs()
{
    this->p_unloaded_dll_funcs->setStringList(this->getDllFuncs(false));
    return this->p_unloaded_dll_funcs;
}

QStringList ExJ2534Wrapper::getDllFuncs(bool loaded)
{
    QStringList list;
    /*
    if(loaded)
    {
        list.append("test 1");
        list.append("test 2");
        list.append("test 3");
        list.append("test 4");
        list.append("test 5");
    }*/

    if(this->p_loaded_funcs == nullptr)return list;
    auto cbor = QCborValue::fromCbor(this->p_loaded_funcs->msg.data).toMap();
    auto funcs = cbor.value(PipeMessage::KEY_Param2).toMap();

    if(loaded)
    {
        for(auto val: funcs)
        {
            if(val.second.toBool(false))
            {
                list.append(val.first.toString());
            }
        }
    }
    else
    {
        for(auto val: funcs)
        {
            if(!val.second.toBool(false))
            {
                list.append(val.first.toString());
            }
        }
    }

    return list;
}


void ExJ2534Wrapper::processPipeMessage(PipeMessage msg)
{
    if(msg.timestamp == 0)msg.timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    //qDbg() << "[TIMESTAMP]: " << msg.timestamp;

    auto obj = new TimestampedObject{msg.timestamp, msg};
    QTimer::singleShot(0,[obj, this]{this->addElement(obj);});

    if(msg.data_type == 2)
    {
        auto cbor = QCborValue::fromCbor(msg.data).toMap();
        switch(cbor.value(PipeMessage::KEY_FuncName).toInteger(-1))
        {
        case 0: //constructor
            {
                qDbg() << "[MSG]: J2534DllWrapper constructor: 0x" << QString::number(cbor.value(PipeMessage::KEY_Param1).toInteger(), 16);
            }
            break;
        case 1: // loadDll
            {
                qDbg() << "[MSG]: " << cbor.value(PipeMessage::KEY_Param1).toString();
                if(cbor.contains(PipeMessage::KEY_Error)){
                    obj->hasError = true;
                    qDbg() << "[MSG] loadDll failed: " << cbor.value(PipeMessage::KEY_Error).toString();
                }
                this->p_loaded_funcs = obj;
                emit dll_funcsChanged();
                auto funcs = cbor.value(PipeMessage::KEY_Param2).toMap();
                for(auto val: funcs)
                {
                    qDbg() << "[MAP] " << val.first.toString() << " loaded: " << val.second.toBool();
                }
            }
            break;
        case PipeMessage::KEY_PassThruReadMsgs:
            {
                this->p_read_msgs->processPipeMessage(msg);
            }
            break;
        case PipeMessage::KEY_PassThruWriteMsgs:
            {
                this->p_write_msgs->processPipeMessage(msg);
            }
            break;
        case PipeMessage::KEY_PassThruQueueMsgs:
            {
                qDbg() << "[QUEUE_MSG] ";
            }
            break;
        }
    }
}

void ExJ2534Wrapper::exportToJSON()
{
    QJsonDocument doc;
    QJsonArray arr;
    for(auto it: this->p_objs)
    {        
        arr.append(
            QJsonObject
            {
                {"id", (qint64)it->msg.id},
                {"data_type", (qint64)it->msg.data_type},
                {"type", (qint64)it->msg.type},
                {"timestamp", (qint64)it->msg.timestamp},
                {"path", it->msg.path},
                {"message", it->msg.message},
                {"data", QCborValue::fromCbor(it->msg.data).toJsonValue()},
            }
        );
    }
    doc.setArray(arr);

    auto path = QGuiApplication::instance()->applicationDirPath().append(QStringLiteral("\\export.json"));
    QFile file(path);
    if(file.open(QFile::WriteOnly))
    {
        file.write(doc.toJson());
        file.close();
    }
}

void ExJ2534Wrapper::loadFromJSON(QString fpath)
{
    QString path;
    if(!fpath.isEmpty())path = fpath;
    else path = QGuiApplication::instance()->applicationDirPath().append(QStringLiteral("\\export.json"));

    QFile file(path);
    if(file.open(QFile::ReadOnly))
    {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        file.close();

        auto arr = doc.array();
        for(auto it: arr)
        {
            PipeMessage pmsg;
            auto obj = it.toObject();
            pmsg.id = obj.value("id").toInteger();
            pmsg.data_type = obj.value("data_type").toInteger();
            pmsg.type = obj.value("type").toInteger();
            pmsg.timestamp = obj.value("timestamp").toInteger();
            pmsg.path = obj.value("path").toString();
            pmsg.message = obj.value("message").toString();
            pmsg.data = QCborValue::fromJsonValue(obj.value("data")).toCbor();

            this->processPipeMessage(pmsg);
        }
    }
}

void ExJ2534Wrapper::addElement(TimestampedObject * obj)
{
    //qDbg() << "[addElement]: " << obj->msg.message;
    this->beginInsertRows(QModelIndex(), 0, 0);
    this->p_objs.insert(0, obj);
    this->endInsertRows();
}

void ExJ2534Wrapper::removeElement(TimestampedObject * obj)
{
    auto idx = this->p_objs.indexOf(obj);
    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->p_objs.removeOne(obj);
    this->endRemoveRows();
}

void ExJ2534Wrapper::removeElementByIndex(qint32 idx)
{
    auto obj = this->p_objs.at(idx);
    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->p_objs.removeOne(obj);
    this->endRemoveRows();
}


QVariant ExJ2534Wrapper::data(const QModelIndex & index, int role) const
{
    if (index.row() < 0 || index.row() > this->p_objs.size())return QVariant();

    auto obj = this->p_objs[index.row()];

    switch (role) {
    case ID:
        return obj->msg.id;
    case Timestamp:
        return QDateTime::fromMSecsSinceEpoch(obj->timestamp).toString("[hh:mm:ss.zzz]");
    case Message:
        return obj->msg.message;
    case HasError:
        return obj->hasError;
    }

    return QVariant();
}
