QT += quick widgets network core
QT += quickcontrols2

TEMPLATE = app
CONFIG += qmltypes
CONFIG += c++latest

TARGET = j2534_OverIP_App
# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

DESTDIR = ../build

SOURCES += \
    exj2534emulator.cpp \
    exj2534readmsgslist.cpp \
    exj2534writemsgslist.cpp \
    exmessagepreprocessor.cpp \
    main.cpp \
    expipeserver.cpp

HEADERS += \
    defs.h \
    exj2534emulator.h \
    exj2534readmsgslist.h \
    exj2534writemsgslist.h \
    exmessagepreprocessor.h \
    expipeserver.h  \
    types.h

DEPLOY_TARGET = $$DESTDIR$$QMAKE_DIR_SEP$$TARGET".exe"
WINDEPLOY_CMD = $$shell_quote($$shell_path($$[QT_INSTALL_BINS]$$QMAKE_DIR_SEP"windeployqt.exe"))
win32: QMAKE_POST_LINK = $$WINDEPLOY_CMD --release --ignore-library-errors --compiler-runtime --verbose 2 --qmldir $$shell_quote($$shell_path($$_PRO_FILE_PWD_)) $$DEPLOY_TARGET

message("POST: $$QMAKE_POST_LINK")

qml_resources.files = Main.qml J2534Emulator.qml
qml_resources.prefix = /

RESOURCES += qml_resources

QML_IMPORT_NAME = de.xplatforms.j2534
QML_IMPORT_VERSION = 1.0

DISTFILES += \
    J2534Emulator.qml





