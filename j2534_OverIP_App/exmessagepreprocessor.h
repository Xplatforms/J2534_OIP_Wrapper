#ifndef EXMESSAGEPREPROCESSOR_H
#define EXMESSAGEPREPROCESSOR_H


#include <QObject>
#include <QQmlEngine>
#include "types.h"


class ExMessagePreProcessor : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ExMessagePreProcessor)
    QML_SINGLETON

public:
    static ExMessagePreProcessor * getInstance();
    static ExMessagePreProcessor * create(QQmlEngine *, QJSEngine *engine);

public slots:
    void incommingMessageFromClient(PipeMessage msg);

private:
    explicit ExMessagePreProcessor(QObject *parent = nullptr);

private:
    inline static ExMessagePreProcessor *s_singletonInstance = nullptr;
    inline static QJSEngine *s_engine = nullptr;

};

#endif // EXMESSAGEPREPROCESSOR_H
