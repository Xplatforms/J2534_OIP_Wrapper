#include "defs.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlEngineExtensionPlugin>
#include <QQmlContext>
#include <QSettings>

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

    //ExJ2534SimpleLogger   0 - undefined
    //J2534DllWrapper       2
    //J2534Emulator         3
    //J2534Cannelloni       4

    qint32 module = 0;
    QSettings s("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\PassThruSupport.04.04\\XplatformsPassThruOverIP",QSettings::NativeFormat);
    auto sel_interface = s.value(QLatin1StringView("ExSelectedInterface")).toString();
    if(sel_interface.compare(QLatin1StringView("J2534DllWrapper"), Qt::CaseInsensitive) == 0)module = 2;
    if(sel_interface.compare(QLatin1StringView("J2534Emulator"), Qt::CaseInsensitive) == 0)module = 3;
    if(sel_interface.compare(QLatin1StringView("J2534Cannelloni"), Qt::CaseInsensitive) == 0)module = 4;

    QQmlApplicationEngine engine;    
    engine.rootContext()->setContextProperty(QLatin1StringView("ExSelectedInterface"), QVariant(module));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
        &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.load("qrc:/Main.qml");


    return app.exec();
}
