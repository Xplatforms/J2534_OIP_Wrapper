#ifndef EXJ2534WRAPPER_H
#define EXJ2534WRAPPER_H

#include <QObject>
#include <QAbstractListModel>
#include <QStringListModel>
#include <QQmlEngine>
#include <QJSEngine>

#include "types.h"
#include "exj2534readmsgslist.h"
#include "exj2534writemsgslist.h"



class ExJ2534Wrapper : public QAbstractListModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(ExJ2534Wrapper)
    QML_SINGLETON

public:
    enum ElemRoles
    {
        ID = Qt::UserRole + 1,
        Timestamp,
        Message,
        HasError
    };

    static ExJ2534Wrapper * getInstance();
    static ExJ2534Wrapper *create(QQmlEngine *, QJSEngine *engine);

    Q_PROPERTY(QStringListModel * unloaded_dll_funcs READ unloaded_dll_funcs NOTIFY dll_funcsChanged FINAL);
    Q_PROPERTY(QStringListModel * loaded_dll_funcs READ loaded_dll_funcs NOTIFY dll_funcsChanged FINAL);
    Q_INVOKABLE QStringListModel * loaded_dll_funcs();
    Q_INVOKABLE QStringListModel * unloaded_dll_funcs();

    Q_INVOKABLE ExJ2534ReadMsgsList * getReadMsgsList(){return this->p_read_msgs;}
    Q_INVOKABLE ExJ2534WriteMsgsList * getWriteMsgsList(){return this->p_write_msgs;}

    Q_INVOKABLE void exportToJSON();
    Q_INVOKABLE void loadFromJSON(QString fpath);

    QStringList getDllFuncs(bool loaded = true);

    int rowCount(const QModelIndex & parent = QModelIndex()) const {return this->p_objs.count();}
    QVariant data(const QModelIndex & index, int role = ExJ2534Wrapper::ID) const;
    QHash<int, QByteArray> roleNames() const {return this->m_roles;}

public slots:
    void processPipeMessage(PipeMessage msg);
    void addElement(TimestampedObject * obj);
    void removeElement(TimestampedObject * obj);
    void removeElementByIndex(qint32 idx);

signals:
    void dll_funcsChanged();

private:
    QHash<int, QByteArray>          m_roles;
    QList<TimestampedObject *>      p_objs;
    TimestampedObject *             p_loaded_funcs;
    QStringListModel *              p_unloaded_dll_funcs;
    QStringListModel *              p_loaded_dll_funcs;
    ExJ2534ReadMsgsList *           p_read_msgs;
    ExJ2534WriteMsgsList *          p_write_msgs;

private:
    ExJ2534Wrapper(QObject * parent = nullptr);
    inline static ExJ2534Wrapper *s_ExJ2534Wrapper_singletonInstance = nullptr;
    inline static QJSEngine * s_engine = nullptr;



};

#endif // EXJ2534WRAPPER_H
