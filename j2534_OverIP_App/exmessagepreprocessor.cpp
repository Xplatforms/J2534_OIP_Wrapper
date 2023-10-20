#include "defs.h"
#include <QCborValue>
#include <QCborMap>

#include "exmessagepreprocessor.h"
#include "exj2534emulator.h"


ExMessagePreProcessor::ExMessagePreProcessor(QObject *parent):QObject{parent}
{

}

ExMessagePreProcessor * ExMessagePreProcessor::getInstance()
{
    if(ExMessagePreProcessor::s_singletonInstance == nullptr)ExMessagePreProcessor::s_singletonInstance = new ExMessagePreProcessor();
    return ExMessagePreProcessor::s_singletonInstance;
}

void ExMessagePreProcessor::incommingMessageFromClient(PipeMessage msg)
{
    qDbg() << "[MSG]:" << msg.path << " id:" << msg.id << " type:" << msg.type << " message:" << msg.message << " data size:" << msg.data.size();
    if(msg.data_type == 1)
    {
        auto cb_map = QCborValue::fromCbor(msg.data).toMap();
        qDbg() << "[DATA] " << cb_map;
    }

    switch(msg.type)
    {
    case 3: // J2534Emulator
        ExJ2534Emulator::getInstance()->processPipeMessage(msg);
        break;
    }

}

ExMessagePreProcessor * ExMessagePreProcessor::create(QQmlEngine *, QJSEngine *engine)
{
    // The instance has to exist before it is used. We cannot replace it.
    Q_ASSERT(s_singletonInstance);

    // The engine has to have the same thread affinity as the singleton.
    Q_ASSERT(engine->thread() == s_singletonInstance->thread());

    // There can only be one engine accessing the singleton.
    if (s_engine)
        Q_ASSERT(engine == s_engine);
    else
        s_engine = engine;

    // Explicitly specify C++ ownership so that the engine doesn't delete
    // the instance.
    QJSEngine::setObjectOwnership(s_singletonInstance, QJSEngine::CppOwnership);
    return s_singletonInstance;
}
