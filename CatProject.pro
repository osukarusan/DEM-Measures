#-------------------------------------------------
#
# Project created by QtCreator 2016-11-09T12:22:02
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CatProject
TEMPLATE = app
QMAKE_LFLAGS_RELEASE += -static-libgcc -static-libstdc++

INCLUDEPATH += ./glm/

SOURCES += main.cpp\
        mainwindow.cpp \
    terrainviewer.cpp \
    heightstileset.cpp \
    heightsgrid.cpp \
    loaderply.cpp

HEADERS  += mainwindow.h \
    terrainviewer.h \
    heightstileset.h \
    heightsgrid.h \
    loaderply.h \
    utils.h

FORMS    += mainwindow.ui
