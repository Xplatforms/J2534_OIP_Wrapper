#ifndef EXJ2534CANNELLONI_H
#define EXJ2534CANNELLONI_H

#include <QObject>
#include <QAbstractListModel>
#include <QQmlEngine>
#include <QJSEngine>

#include "types.h"
#include "exj2534readmsgslist.h"
#include "exj2534writemsgslist.h"

class ExJ2534Cannelloni : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ExJ2534Cannelloni)
    QML_SINGLETON

public:
    enum ElemRoles
    {
        ID = Qt::UserRole + 1,
        Timestamp,
        Message,
        DATA
    };

    static ExJ2534Cannelloni * getInstance();
    static ExJ2534Cannelloni *create(QQmlEngine *, QJSEngine *engine);

    int rowCount(const QModelIndex & parent = QModelIndex()) const {return this->p_objs.count();}
    QVariant data(const QModelIndex & index, int role = ExJ2534Cannelloni::ID) const;
    QHash<int, QByteArray> roleNames() const {return this->m_roles;}

    Q_INVOKABLE ExJ2534ReadMsgsList * getReadMsgsList(){return this->p_read_msgs;}
    Q_INVOKABLE ExJ2534WriteMsgsList * getWriteMsgsList(){return this->p_write_msgs;}

    Q_INVOKABLE void exportToJSON();
    Q_INVOKABLE void loadFromJSON(QString fpath);


public slots:
    void processPipeMessage(PipeMessage msg);
    void addElement(TimestampedObject * obj);
    void removeElement(TimestampedObject * obj);
    void removeElementByIndex(qint32 idx);

private:
    QHash<int, QByteArray>          m_roles;
    QList<TimestampedObject *>      p_objs;
    ExJ2534ReadMsgsList *           p_read_msgs;
    ExJ2534WriteMsgsList *          p_write_msgs;

private:
    ExJ2534Cannelloni(QObject * parent = nullptr);
    inline static ExJ2534Cannelloni *s_ExJ2534Cannelloni_singletonInstance = nullptr;
    inline static QJSEngine * s_engine = nullptr;
};

#endif // EXJ2534CANNELLONI_H
