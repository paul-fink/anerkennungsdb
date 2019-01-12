#-------------------------------------------------
#
# Project created by QtCreator 2016-11-01T12:40:02
#
#-------------------------------------------------

QT       += core gui sql printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = anerkennungen
TEMPLATE = app

TRANSLATIONS += ./translations/anerkennungsdb_de.ts

SOURCES += main.cpp\
        mainwindow.cpp \
    database.cpp \
    modifydialog.cpp \
    transferadddialog.cpp \
    aboutdialog.cpp \
    configdialog.cpp \
    configmanager.cpp \
    csvwriter.cpp \
    tableprinter.cpp \
    printlayout.cpp

HEADERS  += mainwindow.h \
    database.h \
    modifydialog.h \
    transferadddialog.h \
    aboutdialog.h \
    configdialog.h \
    configmanager.h \
    csvwriter.h \
    tableprinter.h \
    printlayout.h \
    version.h

FORMS    += mainwindow.ui \
    modifydialog.ui \
    transferadddialog.ui \
    aboutdialog.ui \
    configdialog.ui

wince*: {
    DEPLOYMENT_PLUGIN += qsqlite
}

RESOURCES += \
    ressources.qrc

win32: RC_ICONS = images/akdb.ico

# QuaZIP is precompiled in folder 3rdparty/lib 
# and according header in 3rdparty/include below the project directory
# INCLUDEPATH += $$PWD/3rdparty/include/quazip
# LIBS += -L$$PWD/3rdparty/lib -lquazip

# Usage of system wide installed and accessible QuaZIP
 LIBS += -lquazip5
 INCLUDEPATH += /usr/include/quazip5
