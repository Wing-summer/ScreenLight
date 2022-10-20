QT += core gui dtkwidget dbus

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT += x11extras
LIBS += -lX11 -lXext -lXtst

TARGET = ScreenLight
TEMPLATE = app

SOURCES += \
        main.cpp \
    maindialog.cpp \
    QHotkey/qhotkey.cpp \
    QHotkey/qhotkey_x11.cpp

RESOURCES +=         resources.qrc

HEADERS += \
    maindialog.h \
    common.h \
    QHotkey/qhotkey.h \
    QHotkey/qhotkey_p.h

LIBS+= -lsystemd

TRANSLATIONS += \
    $$PWD/lang/zh.ts
