#ifndef VEHICLESERVER_H
#define VEHICLESERVER_H

#include <QObject>
#include <QPointer>
#include <QTimer>

class QLocalServer;
class QLocalSocket;
class QProcess;
class QWebSocket;
class QWebSocketServer;

namespace j2534::remote::HelloResult_QtProtobufNested {
enum class Status : int32_t;
}

// Vehicle PC side of the remote link. Listens for one Diagnostic PC on a
// localhost WebSocket (published by cloudflared), checks its Hello, starts
// j2534_device_host.exe for the real driver and relays frames between them.
class VehicleServer : public QObject
{
    Q_OBJECT

public:
    struct Options
    {
        QString driverPath;         // the real (32-bit) J2534 driver
        QString deviceHostPath;     // j2534_device_host.exe
        quint16 port = 0;           // 0 = any free port
        QString secret;             // from the connection code
        QString adapterName;        // shown on the Diagnostic PC
    };

    explicit VehicleServer(QObject *parent = nullptr);
    ~VehicleServer() override;

    bool start(const Options &options, QString *error);
    void stop();

    bool listening() const;
    quint16 port() const;
    bool hasClient() const { return m_authenticated; }
    QString clientName() const { return m_clientName; }

    // Random secret for a connection code, e.g. "K7P4-9QXM-2M8D".
    static QString generateSecret();

    static constexpr int kMaxSecretFailures = 3;
    static constexpr int kHostStartTimeoutMs = 15000;

signals:
    void clientConnected(const QString &name);
    void clientDisconnected();
    void clientRefused(const QString &reason);
    void logMessage(const QString &text);

private:
    void onNewConnection();
    void onClientMessage(const QByteArray &data);
    void onClientDisconnected();
    void onHostConnected();
    void onHostReadyRead();
    void onHostFinished(int exitCode);

    void handleHello(const QByteArray &data);
    void startHost();
    void stopHost();
    using HelloStatus = j2534::remote::HelloResult_QtProtobufNested::Status;
    void refuse(QWebSocket *socket, HelloStatus status, const QString &message);
    void accept();
    void sendHelloResult(QWebSocket *socket, HelloStatus status, const QString &message);
    void log(const QString &text);

    Options m_options;
    QWebSocketServer *m_server = nullptr;
    QPointer<QWebSocket> m_client;
    QString m_clientName;
    bool m_authenticated = false;
    int m_secretFailures = 0;

    QLocalServer *m_pipeServer = nullptr;
    QPointer<QLocalSocket> m_host;
    QProcess *m_hostProcess = nullptr;
    QByteArray m_hostBuffer;
    QTimer m_hostStartTimer;
};

#endif // VEHICLESERVER_H
