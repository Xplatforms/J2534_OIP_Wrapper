#include "vehicleserver.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QHostAddress>
#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>
#include <QProtobufSerializer>
#include <QRandomGenerator>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QtEndian>

#include "j2534_remote.qpb.h"

using namespace j2534::remote;

namespace {

// First byte of an encoded Frame = its oneof field tag (field << 3 | 2).
constexpr char kTagHello = 0x0A;
constexpr char kTagRequest = 0x1A;
constexpr char kTagResponse = 0x22;
constexpr char kTagPing = 0x32;

bool sameSecret(const QByteArray &a, const QByteArray &b)
{
    // Constant time for equal lengths; a length mismatch is not a secret.
    if (a.size() != b.size())
        return false;
    unsigned char diff = 0;
    for (qsizetype i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    return diff == 0;
}

QByteArray lengthPrefixed(const QByteArray &frame)
{
    QByteArray out(4, '\0');
    qToLittleEndian<quint32>(static_cast<quint32>(frame.size()), out.data());
    return out + frame;
}

} // namespace

VehicleServer::VehicleServer(QObject *parent)
    : QObject(parent)
{
    m_hostStartTimer.setSingleShot(true);
    connect(&m_hostStartTimer, &QTimer::timeout, this, [this] {
        if (!m_authenticated && m_client)
            refuse(m_client, HelloResult::Status::STATUS_REJECTED, QStringLiteral("The device host did not start."));
    });
}

VehicleServer::~VehicleServer()
{
    stop();
}

QString VehicleServer::generateSecret()
{
    // Crockford-style alphabet without 0/O, 1/I/L: easy to read out loud.
    static const char alphabet[] = "23456789ABCDEFGHJKMNPQRSTUVWXYZ";
    auto *random = QRandomGenerator::system();
    QString secret;
    for (int i = 0; i < 12; ++i) {
        if (i > 0 && i % 4 == 0)
            secret += QLatin1Char('-');
        secret += QLatin1Char(alphabet[random->bounded(int(sizeof(alphabet) - 1))]);
    }
    return secret;
}

bool VehicleServer::start(const Options &options, QString *error)
{
    stop();
    m_options = options;
    m_secretFailures = 0;

    if (!QFileInfo::exists(options.deviceHostPath)) {
        if (error) *error = QStringLiteral("Device host not found: %1").arg(options.deviceHostPath);
        return false;
    }
    if (options.secret.isEmpty()) {
        if (error) *error = QStringLiteral("No secret set.");
        return false;
    }

    m_server = new QWebSocketServer(QStringLiteral("J2534OverIP"), QWebSocketServer::NonSecureMode, this);
    // Only cloudflared on this PC talks to it; TLS ends at Cloudflare.
    if (!m_server->listen(QHostAddress::LocalHost, options.port)) {
        if (error) *error = QStringLiteral("Can't listen on port %1: %2").arg(options.port).arg(m_server->errorString());
        delete m_server;
        m_server = nullptr;
        return false;
    }
    connect(m_server, &QWebSocketServer::newConnection, this, &VehicleServer::onNewConnection);

    m_pipeServer = new QLocalServer(this);
    m_pipeServer->setSocketOptions(QLocalServer::UserAccessOption);
    connect(m_pipeServer, &QLocalServer::newConnection, this, &VehicleServer::onHostConnected);

    log(QStringLiteral("Vehicle PC listening on 127.0.0.1:%1").arg(m_server->serverPort()));
    return true;
}

void VehicleServer::stop()
{
    stopHost();
    if (m_client) {
        m_client->abort();
        m_client->deleteLater();
    }
    m_client = nullptr;
    m_authenticated = false;
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
    if (m_pipeServer) {
        m_pipeServer->close();
        m_pipeServer->deleteLater();
        m_pipeServer = nullptr;
    }
}

bool VehicleServer::listening() const
{
    return m_server && m_server->isListening();
}

quint16 VehicleServer::port() const
{
    return m_server ? m_server->serverPort() : 0;
}

void VehicleServer::log(const QString &text)
{
    emit logMessage(text);
}

void VehicleServer::onNewConnection()
{
    while (QWebSocket *socket = m_server->nextPendingConnection()) {
        connect(socket, &QWebSocket::disconnected, socket, &QObject::deleteLater);

        if (m_secretFailures >= kMaxSecretFailures) {
            refuse(socket, HelloResult::Status::STATUS_REJECTED,
                   QStringLiteral("Too many wrong connection codes. Restart sharing on the Vehicle PC."));
            continue;
        }
        if (m_client) {
            refuse(socket, HelloResult::Status::STATUS_BUSY, QString());
            continue;
        }

        m_client = socket;
        m_authenticated = false;
        connect(socket, &QWebSocket::binaryMessageReceived, this, &VehicleServer::onClientMessage);
        connect(socket, &QWebSocket::disconnected, this, &VehicleServer::onClientDisconnected);
    }
}

void VehicleServer::onClientMessage(const QByteArray &data)
{
    if (data.isEmpty())
        return;

    if (!m_authenticated) {
        if (data[0] == kTagHello && !m_hostProcess)
            handleHello(data);
        return;   // nothing else before Hello is accepted
    }

    switch (data[0]) {
    case kTagRequest:
        if (m_host)
            m_host->write(lengthPrefixed(data));
        break;
    case kTagPing: {
        QProtobufSerializer serializer;
        Frame ping;
        if (!ping.deserialize(&serializer, data))
            break;
        Pong pong;
        pong.setSentUs(ping.ping().sentUs());
        Frame reply;
        reply.setPong(pong);
        m_client->sendBinaryMessage(reply.serialize(&serializer));
        break;
    }
    default:
        break;
    }
}

void VehicleServer::handleHello(const QByteArray &data)
{
    QProtobufSerializer serializer;
    Frame frame;
    if (!frame.deserialize(&serializer, data) || frame.kindField() != Frame::KindFields::Hello) {
        refuse(m_client, HelloResult::Status::STATUS_REJECTED, QStringLiteral("Malformed Hello."));
        return;
    }

    const Hello hello = frame.hello();
    if (hello.version() != VersionGadget::Version::VERSION_CURRENT) {
        refuse(m_client, HelloResult::Status::STATUS_VERSION_MISMATCH,
               QStringLiteral("Update the Diagnostic PC and the Vehicle PC to the same version."));
        return;
    }
    if (!sameSecret(hello.secret().toUtf8(), m_options.secret.toUtf8())) {
        ++m_secretFailures;
        log(QStringLiteral("Wrong connection code from %1 (%2 of %3)")
                .arg(hello.clientName()).arg(m_secretFailures).arg(kMaxSecretFailures));
        refuse(m_client, HelloResult::Status::STATUS_BAD_SECRET, QString());
        return;
    }

    m_clientName = hello.clientName().left(80);
    log(QStringLiteral("%1 connected, starting the device host").arg(m_clientName));
    startHost();
}

void VehicleServer::startHost()
{
    const QString pipeName = QStringLiteral("j2534_vehicle_%1_%2")
                                 .arg(QCoreApplication::applicationPid())
                                 .arg(QRandomGenerator::global()->generate(), 8, 16, QLatin1Char('0'));
    QLocalServer::removeServer(pipeName);
    if (!m_pipeServer->listen(pipeName)) {
        refuse(m_client, HelloResult::Status::STATUS_REJECTED, QStringLiteral("Can't create the device host pipe."));
        return;
    }

    m_hostBuffer.clear();
    m_hostProcess = new QProcess(this);
    connect(m_hostProcess, &QProcess::finished, this, [this](int exitCode) { onHostFinished(exitCode); });
    m_hostProcess->start(m_options.deviceHostPath,
                         {QStringLiteral("--pipe"), m_pipeServer->fullServerName(),
                          QStringLiteral("--driver"), m_options.driverPath});
    m_hostStartTimer.start(kHostStartTimeoutMs);
}

void VehicleServer::stopHost()
{
    m_hostStartTimer.stop();
    if (m_host) {
        m_host->abort();   // the host sees a broken pipe and closes the driver
        m_host->deleteLater();
    }
    m_host = nullptr;
    if (m_pipeServer)
        m_pipeServer->close();

    if (m_hostProcess) {
        QProcess *process = m_hostProcess;
        m_hostProcess = nullptr;
        process->disconnect(this);
        // Let it disconnect channels and close the device; kill only if stuck.
        connect(process, &QProcess::finished, process, &QObject::deleteLater);
        QTimer::singleShot(5000, process, [process] {
            if (process->state() != QProcess::NotRunning)
                process->kill();
        });
    }
    m_hostBuffer.clear();
}

void VehicleServer::onHostConnected()
{
    QLocalSocket *socket = m_pipeServer->nextPendingConnection();
    if (!socket)
        return;
    if (m_host) {   // only the process we started
        socket->abort();
        socket->deleteLater();
        return;
    }
    m_host = socket;
    m_pipeServer->close();   // no more connections
    connect(socket, &QLocalSocket::readyRead, this, &VehicleServer::onHostReadyRead);
}

void VehicleServer::onHostReadyRead()
{
    m_hostBuffer += m_host->readAll();

    while (m_hostBuffer.size() >= 4) {
        const quint32 size = qFromLittleEndian<quint32>(m_hostBuffer.constData());
        if (size > (1u << 20)) {   // corrupt stream
            log(QStringLiteral("Device host sent a corrupt frame; closing the session."));
            if (m_client) m_client->close();
            return;
        }
        if (m_hostBuffer.size() < qsizetype(4 + size))
            return;
        const QByteArray frame = m_hostBuffer.mid(4, size);
        m_hostBuffer.remove(0, 4 + size);

        if (!m_authenticated) {
            // First frame: the host's ready/failed response (id 0).
            QProtobufSerializer serializer;
            Frame ready;
            if (ready.deserialize(&serializer, frame) && ready.kindField() == Frame::KindFields::Response
                && quint32(ready.response().status()) == 0 && ready.response().error().isEmpty()) {
                accept();
            } else {
                const QString reason = ready.response().error().isEmpty()
                                           ? QStringLiteral("The device host failed to start.")
                                           : ready.response().error();
                refuse(m_client, HelloResult::Status::STATUS_REJECTED, reason);
            }
            continue;
        }

        if (m_client && !frame.isEmpty() && frame[0] != kTagHello)
            m_client->sendBinaryMessage(frame);
    }
}

void VehicleServer::onHostFinished(int exitCode)
{
    log(QStringLiteral("Device host exited (%1)").arg(exitCode));
    if (m_hostProcess) {
        m_hostProcess->deleteLater();
        m_hostProcess = nullptr;
    }
    if (m_client) {
        if (!m_authenticated)
            refuse(m_client, HelloResult::Status::STATUS_REJECTED,
                   QStringLiteral("The device host stopped (exit code %1).").arg(exitCode));
        else
            m_client->close(QWebSocketProtocol::CloseCodeGoingAway, QStringLiteral("Device host stopped"));
    }
}

void VehicleServer::accept()
{
    m_hostStartTimer.stop();
    m_authenticated = true;
    sendHelloResult(m_client, HelloResult::Status::STATUS_ACCEPTED, QString());
    log(QStringLiteral("%1 is using the adapter").arg(m_clientName));
    emit clientConnected(m_clientName);
}

void VehicleServer::refuse(QWebSocket *socket, HelloStatus status, const QString &message)
{
    if (!socket)
        return;
    sendHelloResult(socket, status, message);
    if (socket == m_client) {
        stopHost();
        m_client = nullptr;
        m_authenticated = false;
        emit clientRefused(message);
    }
    socket->flush();
    socket->close(QWebSocketProtocol::CloseCodePolicyViolated, QString());
}

void VehicleServer::sendHelloResult(QWebSocket *socket, HelloStatus status, const QString &message)
{
    HelloResult result;
    result.setStatus(status);
    result.setVersion(VersionGadget::Version::VERSION_CURRENT);
    result.setAdapterName(m_options.adapterName.left(79));
    result.setMessage(message.left(255));
    Frame frame;
    frame.setHelloResult(result);
    QProtobufSerializer serializer;
    socket->sendBinaryMessage(frame.serialize(&serializer));
}

void VehicleServer::onClientDisconnected()
{
    auto *socket = qobject_cast<QWebSocket *>(sender());
    if (socket != m_client)
        return;
    const bool wasConnected = m_authenticated;
    m_client = nullptr;
    m_authenticated = false;
    stopHost();
    if (wasConnected) {
        log(QStringLiteral("%1 disconnected").arg(m_clientName));
        emit clientDisconnected();
    }
}
