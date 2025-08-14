#ifndef MSGSTABLEVIEWMODEL_H
#define MSGSTABLEVIEWMODEL_H

#include <QAbstractTableModel>
#include <QObject>
#include <QQuickItem>

class MSGSTableViewModel : public QAbstractTableModel
{
    Q_OBJECT
    QML_NAMED_ELEMENT(MSGSTableViewModel)
public:
    enum{header_name = Qt::UserRole+1};

    explicit MSGSTableViewModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex & = QModelIndex()) const override
    {
        return 200;
    }

    int columnCount(const QModelIndex & = QModelIndex()) const override
    {
        return 200;
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        switch (role) {
        case Qt::DisplayRole:
            return QString("%1, %2").arg(index.column()).arg(index.row());
        case header_name:
            return QString("head %1 %2").arg(index.column()).arg(index.row());
            break;
        default:
            break;
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return { {Qt::DisplayRole, "display"}, {header_name, "header_name"} };
    }
};

#endif // MSGSTABLEVIEWMODEL_H
