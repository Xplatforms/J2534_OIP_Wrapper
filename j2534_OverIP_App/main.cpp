#include "defs.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngineExtensionPlugin>
#include <QQmlContext>
#include <cstring>

#include "expipeserver.h"
#include "exj2534emulator.h"
#include "exj2534wrapper.h"
#include "exj2534cannelloni.h"
#include "passthruregistry.h"
#include "vehicleserver.h"

#include <QCommandLineParser>
#include <QDir>
#include <QFileInfo>
#include <cstdio>

// --vehicle-server --driver <dll> [--device-host <exe>] [--port <n>]
//                  [--secret <s>] [--adapter-name <name>]
static int runVehicleServer(const QStringList &arguments)
{
    QCommandLineParser parser;
    const QCommandLineOption serverOption(QStringLiteral("vehicle-server"));
    const QCommandLineOption driverOption(QStringLiteral("driver"), {}, QStringLiteral("dll"));
    const QCommandLineOption hostOption(QStringLiteral("device-host"), {}, QStringLiteral("exe"));
    const QCommandLineOption portOption(QStringLiteral("port"), {}, QStringLiteral("port"), QStringLiteral("47534"));
    const QCommandLineOption secretOption(QStringLiteral("secret"), {}, QStringLiteral("secret"));
    const QCommandLineOption nameOption(QStringLiteral("adapter-name"), {}, QStringLiteral("name"));
    parser.addOptions({serverOption, driverOption, hostOption, portOption, secretOption, nameOption});
    if (!parser.parse(arguments) || !parser.isSet(driverOption)) {
        std::fprintf(stderr, "usage: --vehicle-server --driver <dll> [--device-host <exe>] [--port <n>] [--secret <s>]\n");
        return 2;
    }

    VehicleServer::Options options;
    options.driverPath = parser.value(driverOption);
    // Install layout: dist\x64\bin\<App> next to dist\x86\j2534_device_host.exe
    options.deviceHostPath = parser.isSet(hostOption)
        ? parser.value(hostOption)
        : QDir::cleanPath(QCoreApplication::applicationDirPath() + QStringLiteral("/../../x86/j2534_device_host.exe"));
    options.port = static_cast<quint16>(parser.value(portOption).toUInt());
    options.secret = parser.isSet(secretOption) ? parser.value(secretOption) : VehicleServer::generateSecret();
    options.adapterName = parser.isSet(nameOption) ? parser.value(nameOption) : QFileInfo(options.driverPath).baseName();

    VehicleServer server;
    QObject::connect(&server, &VehicleServer::logMessage, [](const QString &text) {
        std::fprintf(stdout, "%s\n", qPrintable(text));
        std::fflush(stdout);
    });
    QString error;
    if (!server.start(options, &error)) {
        std::fprintf(stderr, "%s\n", qPrintable(error));
        return 1;
    }
    std::fprintf(stdout, "Connection code (local): 127.0.0.1:%u#%s\n", server.port(), qPrintable(options.secret));
    std::fflush(stdout);
    return QCoreApplication::exec();
}

int main(int argc, char *argv[])
{
    // Elevated helper started by InterfaceConfig::apply(): write the registry, no UI.
    if (argc > 1 && std::strcmp(argv[1], "--apply-config") == 0)
    {
        QCoreApplication helper(argc, argv);
        return PassThruRegistry::runApplyFromArguments(helper.arguments());
    }

    // Vehicle PC without UI (also used by the end-to-end test).
    if (argc > 1 && std::strcmp(argv[1], "--vehicle-server") == 0)
    {
        QCoreApplication headless(argc, argv);
        return runVehicleServer(headless.arguments());
    }

    QGuiApplication app(argc, argv);

    ExPipeServer * server = ExPipeServer::getInstance();//"\\\\.\\pipe\\exj2534_overip_pipe");
    if (!server->startServer("\\\\.\\pipe\\exj2534_overip_pipe")) {
        qDbg() << "Another instance is already running";
    }
    else qDbg() << "Server started";


    qmlRegisterSingletonInstance("ExJ2534Emulator", 1, 0, "ExJ2534Emulator", ExJ2534Emulator::getInstance());
    qmlRegisterSingletonInstance("ExJ2534Wrapper", 1, 0, "ExJ2534Wrapper", ExJ2534Wrapper::getInstance());
    qmlRegisterSingletonInstance("ExJ2534Cannelloni", 1, 0, "ExJ2534Cannelloni", ExJ2534Cannelloni::getInstance());

    // The selected PassThru interface comes from the InterfaceConfig QML singleton.
    QQmlApplicationEngine engine;

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("de.xplatforms.j2534", "Main");


    return app.exec();
}
