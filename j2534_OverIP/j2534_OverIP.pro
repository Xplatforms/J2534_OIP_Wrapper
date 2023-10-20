CONFIG += -qt

TEMPLATE = lib
DEFINES += J2534OVERIP_LIBRARY

CONFIG += c++latest

DEF_FILE=exports.def
DISTFILES += exports.def

DESTDIR = ../build

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
    cbor_utils.cpp \
    exj2534device.cpp \
    exj2534oipsocks.cpp \
    expipeclient.cpp \
    j2534.cpp \
    j2534dllwrapper.cpp \
    j2534emulator.cpp \
    j2534global.cpp \
    j2534simplelogger.cpp \
    main.cpp \
    ExDbg.cpp


HEADERS += \
    Exj2534WrapperInterface.h \
    cbor_utils.h \
    exj2534device.h \
    exj2534oipsocks.h \
    expipeclient.h \
    j2534_OverIP_global.h \
    j2534.h \
    j2534_defs.h \
    j2534_dev_typedefs.h \
    ExDbg.h \
    j2534dllwrapper.h \
    j2534emulator.h \
    j2534global.h \
    j2534simplelogger.h \
    timestamp_util.h

LIBS += -lws2_32 -lAdvapi32

LIBS += -L../build
LIBS += -lcn-cbor

message("LIBS = $$LIBS")


#win32: CONFIG(release, debug|release){
#    LIBS += -L$$_PRO_FILE_PWD_/lib/Release
#    DEPLOY_TARGET = $$shell_quote($$shell_path($$DESTDIR"/"$$TARGET".dll"))
#}
#win32: CONFIG(debug, debug|release){
#    LIBS += -L$$_PRO_FILE_PWD_/lib/Debug
#    DEPLOY_TARGET = $$shell_quote($$shell_path($$shadowed($$PWD)"/debug/"$$TARGET".dll"))
#}

#WINDEPLOY_CMD = $$shell_quote($$shell_path($$[QT_INSTALL_BINS]$$QMAKE_DIR_SEP"windeployqt.exe"))
#win32: QMAKE_POST_LINK = $$WINDEPLOY_CMD --compiler-runtime --verbose 2 --release --angle --qmldir $$shell_quote($$shell_path($$_PRO_FILE_PWD_)) $$DEPLOY_TARGET
#
#message("QMAKE_POST_LINK: $$QMAKE_POST_LINK")

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target
