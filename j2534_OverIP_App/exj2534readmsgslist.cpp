#include "defs.h"
#include "exj2534readmsgslist.h"
#include "types.h"

#include <QDateTime>
#include <QCborValue>
#include <QCborMap>
#include <QCborArray>

ExJ2534ReadMsgsList::ExJ2534ReadMsgsList(QObject *parent)
    : QAbstractListModel{parent}
{
    this->m_roles[ID] = "ID";
    this->m_roles[HasError] = "HasError";
    this->m_roles[ProtocolID] = "ProtocolID";
    this->m_roles[RxStatus] = "RxStatus";
    this->m_roles[TxFlags] = "TxFlags";
    this->m_roles[Timestamp] = "Timestamp";
    this->m_roles[DataSize] = "DataSize";
    this->m_roles[ExtraDataIndex] = "ExtraDataIndex";
    this->m_roles[Data] = "Data";
}

QVariant ExJ2534ReadMsgsList::data(const QModelIndex & index, int role) const
{
    //qDbg() << "[RX_GET_DATA] index " << index << " role " << role << " objs_count " << this->p_objs.count();

    if (index.row() < 0 || index.row() > this->p_objs.size())return QVariant();
    auto obj = this->p_objs[index.row()];
    //qDbg() << "[OBJ] " << obj->ProtocolID << obj->RxStatus << obj->DataSize << obj->Data;

    switch (role)
    {
    case ID:
        return QString::number(obj->Data[0], 16);
    case ProtocolID:
        //qDbg() << "[ProtocolID] " << obj->ProtocolID;
        //qDbg() << "[ProtocolID2] " << QString::number(obj->ProtocolID, 16);
        return QString::number(obj->ProtocolID, 16);
    case RxStatus:
        //qDbg() << "[RxStatus] " << obj->RxStatus;
        //qDbg() << "[RxStatus2] " << QString::number(obj->RxStatus, 16);
        return QString::number(obj->RxStatus, 16);
    case TxFlags:
        return QString::number(obj->TxFlags, 16);
    case Timestamp:
        return QDateTime::fromMSecsSinceEpoch(obj->Timestamp).toString("[hh:mm:ss.zzz]");
    case DataSize:
        return QVariant((quint32)obj->DataSize);
    case ExtraDataIndex:
        return QVariant((quint32)obj->ExtraDataIndex);
    case Data:
        {
            QString hex;
            for(int y = 0; y < obj->DataSize; y++)
            {
                hex.append(QLatin1String(" ") + QString::number(obj->Data[y], 16));
                //printf("%02x ", obj->Data[y]);
            }
            return hex;
        }
    case HasError:
        return false;
    }

    return QVariant();
}


/*
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_FuncName , ExPipeClient::KEY_PassThruReadMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param1 , ChannelID);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param2, 0);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);
    cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 0);

    if (this->m_PassThruReadMsgs) {
        retval = this->m_PassThruReadMsgs(ChannelID, pMsg, pNumMsgs, Timeout);
        cbor_utils::map_put_PASSTHRU_MSG(cb_map_root, ExPipeClient::KEY_Param2, *pNumMsgs, pMsg);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param3, *pNumMsgs);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_Param4, Timeout);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RX_TX, 1);
        cbor_utils::map_put_int(cb_map_root, ExPipeClient::KEY_RETVAL_LONG, retval);
    } else {
        EXDBG_LOG << "Function " << __FUNCTION__ << " not found in DLL." << EXDBG_END;
        cbor_utils::map_put_string(cb_map_root, ExPipeClient::KEY_Error, std::string("Function ").append(__FUNCTION__).append(" not found in DLL.").c_str());
    }

        cn_cbor* cb_map_elem = cn_cbor_map_create(&cn_errback);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param1, msgs[i].ProtocolID);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param2, msgs[i].RxStatus);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param3, msgs[i].TxFlags);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param4, msgs[i].Timestamp);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param5, msgs[i].DataSize);
        cbor_utils::map_put_int(cb_map_elem, ExPipeClient::KEY_Param6, msgs[i].ExtraDataIndex);
        cbor_utils::map_put_data(cb_map_elem, ExPipeClient::KEY_Param7, msgs[i].Data, msgs[i].DataSize);
        cbor_utils::arr_append_cbor(cb_array, cb_map_elem);
*/

void ExJ2534ReadMsgsList::processPipeMessage(PipeMessage msg)
{
    if(msg.timestamp == 0)msg.timestamp = QDateTime::currentDateTime().toSecsSinceEpoch();
    auto cbor_data = QCborValue::fromCbor(msg.data).toMap();
    //auto obj = new TimestampedObject{msg.timestamp, msg, cbor_data.contains(PipeMessage::KEY_Error)};

    //PipeClient::KEY_Param2
    if(cbor_data.value(PipeMessage::KEY_Param2).isInteger())
    {
        qDbg() << "[READ_MSG]: pMsg is NULL";
    }
    else
    {
        auto cbor_msgs = cbor_data.value(PipeMessage::KEY_Param2).toMap();
        auto msgs_count = cbor_msgs.value(0).toInteger(0);
        auto msgs = cbor_msgs.value(1).toArray();

        for(auto i = 0; i < msgs_count; i++)
        {
            PASSTHRU_MSG * msg = new PASSTHRU_MSG();
            memset(msg, 0, sizeof(PASSTHRU_MSG));
            auto cbor_msg = msgs[i].toMap();
            msg->ProtocolID = cbor_msg.value(PipeMessage::KEY_Param1).toInteger(0);
            msg->RxStatus = cbor_msg.value(PipeMessage::KEY_Param2).toInteger(0);
            msg->TxFlags = cbor_msg.value(PipeMessage::KEY_Param3).toInteger(0);
            msg->Timestamp = cbor_msg.value(PipeMessage::KEY_Param4).toInteger(0);
            msg->DataSize = cbor_msg.value(PipeMessage::KEY_Param5).toInteger(0);
            msg->ExtraDataIndex = cbor_msg.value(PipeMessage::KEY_Param6).toInteger(0);
            auto data = cbor_msg.value(PipeMessage::KEY_Param7).toByteArray();
            if(data.length() != msg->DataSize)
            {
                qDbg() << "[CAN_RX_MSG] data length != DataSize in PASSTH_MSG!";
                memcpy(msg->Data, data.constData(), data.length());
            }
            else
            {
                memcpy(msg->Data, data.constData(), data.length());
            }

            this->addElement(msg);
        }
    }


    //this->addElement(obj);
}

void ExJ2534ReadMsgsList::addElement(PASSTHRU_MSG * obj)
{
    qDbg() << "[ADD_RX_ELEMENT] count " << this->p_objs.count();
    this->beginInsertRows(QModelIndex(), this->rowCount(), this->rowCount());
    this->p_objs.append(obj);
    this->endInsertRows();
}

void ExJ2534ReadMsgsList::removeElement(PASSTHRU_MSG * obj)
{
    auto idx = this->p_objs.indexOf(obj);
    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->p_objs.removeOne(obj);
    this->endRemoveRows();
}

void ExJ2534ReadMsgsList::removeElementByIndex(qint32 idx)
{
    auto obj = this->p_objs.at(idx);
    this->beginRemoveRows(QModelIndex(), idx, idx);
    this->p_objs.removeOne(obj);
    this->endRemoveRows();
}
