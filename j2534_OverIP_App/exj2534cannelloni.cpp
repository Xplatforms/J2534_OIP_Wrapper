#include "exj2534cannelloni.h"
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

ExJ2534Cannelloni::ExJ2534Cannelloni(QObject * parent):QAbstractListModel(parent)
{
    this->m_roles[ID] = "ID";
    this->m_roles[Timestamp] = "Timestamp";
    this->m_roles[Message] = "Message";
    this->m_roles[DATA] = "DATA";

    this->p_read_msgs = new ExJ2534ReadMsgsList(this);
    this->p_write_msgs = new ExJ2534WriteMsgsList(this);
}

ExJ2534Cannelloni * ExJ2534Cannelloni::getInstance()
{
    if(ExJ2534Cannelloni::s_ExJ2534Cannelloni_singletonInstance == nullptr)ExJ2534Cannelloni::s_ExJ2534Cannelloni_singletonInstance = new ExJ2534Cannelloni(nullptr);

    return ExJ2534Cannelloni::s_ExJ2534Cannelloni_singletonInstance;
}

ExJ2534Cannelloni * ExJ2534Cannelloni::create(QQmlEngine *, QJSEngine *engine)
{
    auto instance = ExJ2534Cannelloni::getInstance();
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

void ExJ2534Cannelloni::processPipeMessage(PipeMessage msg)
{
    if(msg.timestamp == 0)msg.timestamp = QDateTime::currentDateTime().toMSecsSinceEpoch();
    auto obj = new TimestampedObject{msg.timestamp, msg};

    bool proceeded = false;

    if(msg.data_type == 2)
    {
        auto cbor = QCborValue::fromCbor(msg.data).toMap();
        qDbg() << "[FuncName] " << cbor.value(PipeMessage::KEY_FuncName).toInteger(-1) << QCborValue::fromCbor(msg.data);

        switch(cbor.value(PipeMessage::KEY_FuncName).toInteger(-1))
        {
        case PipeMessage::KEY_PassThruOpen:
            {
                proceeded = true;
                auto pName = cbor.value(PipeMessage::KEY_Param1).toString();
                qint32 pDeviceID = cbor.value(PipeMessage::KEY_Param2).toInteger(0);
                obj->data = QString("pName = ")+pName+" pDeviceID = " + QString::number(pDeviceID);
                qDbg() << "[OPEN] " << obj->data;
                QTimer::singleShot(0,[this, obj]{this->addElement(obj);});
            }
            break;
        case PipeMessage::KEY_PassThruReadMsgs:
            proceeded = true;
            this->p_read_msgs->processPipeMessage(msg);
            break;
        case PipeMessage::KEY_PassThruWriteMsgs:
            proceeded = true;
            this->p_write_msgs->processPipeMessage(msg);
            break;
        }
    }

    if(!proceeded)
    {
        qDbg() << "[ExJ2534Cannelloni] MSG NOT PROCEEDED! data_type " << msg.data_type << QCborValue::fromCbor(msg.data);
        QTimer::singleShot(0,[this, obj]{this->addElement(obj);});
    }
}


void ExJ2534Cannelloni::exportToJSON()
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

void ExJ2534Cannelloni::loadFromJSON(QString fpath)
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

void ExJ2534Cannelloni::addElement(TimestampedObject * obj)
{
    this->beginInsertRows(QModelIndex(), 0, 0);
    this->p_objs.insert(0, obj);
    this->endInsertRows();
}

void ExJ2534Cannelloni::removeElement(TimestampedObject * obj)
{
    auto idx = this->p_objs.indexOf(obj);
    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->p_objs.removeOne(obj);
    this->endRemoveRows();
}

void ExJ2534Cannelloni::removeElementByIndex(qint32 idx)
{
    auto obj = this->p_objs.at(idx);
    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->p_objs.removeOne(obj);
    this->endRemoveRows();
}

QVariant ExJ2534Cannelloni::data(const QModelIndex & index, int role) const
{
    if (index.row() < 0 || index.row() > this->p_objs.size())return QVariant();

    auto obj = this->p_objs[index.row()];

    switch (role) {
    case ID:
        return obj->msg.id;
    case Timestamp:
        return QDateTime::fromMSecsSinceEpoch(obj->timestamp).toString("hh:mm:ss.zzz");
    case Message:
        return obj->msg.message;
    case DATA:
        return obj->data;
    }

    return QVariant();
}
