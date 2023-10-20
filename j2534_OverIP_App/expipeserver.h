#ifndef EXPIPESERVER_H
#define EXPIPESERVER_H

#include <QLocalServer>
#include <QThread>
#include <QLocalSocket>
#include <QByteArray>
#include <QMutex>
#include <QCborMap>
#include <QCborValue>
#include <QDebug>

class ExPipeServer : public QLocalServer
{
    Q_OBJECT

public:
    explicit ExPipeServer(QObject *parent = nullptr);

    static ExPipeServer* getInstance();

    bool startServer(const QString &serverName);
    void stopServer();

private slots:
    void handleNewConnection();
    void handleSocketReadyRead(QLocalSocket *clientSocket);
    void handleSocketDisconnected();

private:
    static ExPipeServer* instance;
    static QMutex mutex;

    QHash<QLocalSocket*, QByteArray> m_buffers;
    QHash<QLocalSocket*, qint32> m_sizes;

    void processData(QLocalSocket *clientSocket);
};

#endif // EXPIPESERVER_H
