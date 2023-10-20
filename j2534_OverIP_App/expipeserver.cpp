#include "defs.h"
#include "types.h"
#include "expipeserver.h"
#include <QLocalSocket>
#include <QCborMap>
#include <QCborValue>
#include <QTimer>

#include "expipeserver.h"
#include "exmessagepreprocessor.h"

ExPipeServer* ExPipeServer::instance = nullptr;
QMutex ExPipeServer::mutex;

ExPipeServer::ExPipeServer(QObject *parent) : QLocalServer(parent) {
    connect(this, &ExPipeServer::newConnection, this, &ExPipeServer::handleNewConnection);
    //this->setSocketOptions(OtherAccessOption);
}

ExPipeServer* ExPipeServer::getInstance() {
    mutex.lock();
    if (instance == nullptr) {
        instance = new ExPipeServer();
        instance->setSocketOptions(QLocalServer::WorldAccessOption); // Setting the socket option
    }
    mutex.unlock();
    return instance;
}

bool ExPipeServer::startServer(const QString &serverName) {
    if (listen(serverName)) {
        qDbg() << "Server started, listening to" << this->fullServerName();
        return true;
    } else {
        qDbg() << "Server failed to start:" << errorString();
        close();
        return false;
    }
}

void ExPipeServer::stopServer() {
    close();
}

void ExPipeServer::handleNewConnection() {
    qDbg() << "handleNewConnection";
    while (hasPendingConnections()) {
        QLocalSocket *clientSocket = nextPendingConnection();
        connect(clientSocket, &QLocalSocket::readyRead, this, [=](){ handleSocketReadyRead(clientSocket); });
        connect(clientSocket, &QLocalSocket::disconnected, this, &ExPipeServer::handleSocketDisconnected);
    }
}

void ExPipeServer::handleSocketReadyRead(QLocalSocket *clientSocket) {
    processData(clientSocket);
}

void ExPipeServer::handleSocketDisconnected() {
    auto *clientSocket = static_cast<QLocalSocket*>(sender());
    if (clientSocket) {
        m_buffers.remove(clientSocket);
        m_sizes.remove(clientSocket);
        clientSocket->deleteLater();
    }
}

void ExPipeServer::processData(QLocalSocket *clientSocket) {
    qDbg() << "processData";

    QByteArray &buffer = m_buffers[clientSocket];
    qint32 &size = m_sizes[clientSocket];

    while (clientSocket->bytesAvailable()) {
        buffer.append(clientSocket->readAll());

        while ((size == 0 && buffer.size() >= 4) || (size > 0 && buffer.size() >= size)) {
            if (size == 0 && buffer.size() >= 4) {
                size = *reinterpret_cast<const qint32*>(buffer.constData());
                buffer.remove(0, 4);
            }

            if (size > 0 && buffer.size() >= size) {
                QByteArray data = buffer.mid(0, size);
                buffer.remove(0, size);
                size = 0;

                QCborValue cborValue = QCborValue::fromCbor(data);
                if (!cborValue.isMap()) {
                    qDbg() << "Error: Data is not a CBOR map";
                    continue;
                }

                QCborMap map = cborValue.toMap();

                PipeMessage msg;

                if (map.contains(QLatin1String("id")) && map.value("id").isInteger()) {
                    msg.id = map.value("id").toInteger();
                } else {
                    qDbg() << "Error: 'id' field is missing or not an integer";
                }

                if (map.contains(QLatin1String("type")) && map.value("type").isInteger()) {
                    msg.type = map.value("type").toInteger();
                } else {
                    qDbg() << "Error: 'type' field is missing or not an integer";
                }

                if (map.contains(QLatin1String("data_type")) && map.value("data_type").isInteger()) {
                    msg.data_type = map.value("data_type").toInteger();
                } else {
                    qDbg() << "Error: 'type' field is missing or not an integer";
                }

                if (map.contains(QLatin1String("path")) && map.value("path").isString()) {
                    msg.path = map.value("path").toString();
                } else {
                    qDbg() << "Error: 'path' field is missing or not a string";
                }

                if (map.contains(QLatin1String("message")) && map.value("message").isString()) {
                    msg.message = map.value("message").toString();
                } else {
                    qDbg() << "Error: 'message' field is missing or not a string";
                }

                if (map.contains(QLatin1String("data")) && map.value("data").isByteArray()) {
                    msg.data = map.value("data").toByteArray();
                } else {
                    qDbg() << "Error: 'data' field is missing or not a byte array";
                }

                QTimer::singleShot(0,[msg](){ExMessagePreProcessor::getInstance()->incommingMessageFromClient(msg);});
            }
        }
    }
}


