#ifndef DEFS_H
#define DEFS_H

#include <QDebug>

#define qDbg() qDebug() << "["<< (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__) << __func__ << __LINE__ << "]: "



#endif // DEFS_H
