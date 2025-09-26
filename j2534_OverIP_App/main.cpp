#include "defs.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngineExtensionPlugin>

#include "expipeserver.h"
#include "exj2534emulator.h"
#include "exj2534wrapper.h"
#include "exj2534cannelloni.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    ExPipeServer * server = ExPipeServer::getInstance();//"\\\\.\\pipe\\exj2534_overip_pipe");
    if (!server->startServer("\\\\.\\pipe\\exj2534_overip_pipe")) {
        qDbg() << "Another instance is already running";
    }
    else qDbg() << "Server started";


    qmlRegisterSingletonInstance("ExJ2534Emulator", 1, 0, "ExJ2534Emulator", ExJ2534Emulator::getInstance());
    qmlRegisterSingletonInstance("ExJ2534Wrapper", 1, 0, "ExJ2534Wrapper", ExJ2534Wrapper::getInstance());
    qmlRegisterSingletonInstance("ExJ2534Cannelloni", 1, 0, "ExJ2534Cannelloni", ExJ2534Cannelloni::getInstance());


    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load("qrc:/Main.qml");


    return app.exec();
}
