QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += multimedia

CONFIG += c++17 static

LIBS += -Lportaudio.dll

QMAKE_CXX_FLAGS += /EHa

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    animation.cpp \
    animationaudio.cpp \
    createprojectwindow.cpp \
    frame.cpp \
    helper.cpp \
    main.cpp \
    mainwindow.cpp \
    paintingwidget.cpp \
    projectsettingwindow.cpp

HEADERS += \
    animation.h \
    animationaudio.h \
    createprojectwindow.h \
    frame.h \
    helper.h \
    mainwindow.h \
    paintingwidget.h \
    projectsettingwindow.h

FORMS += \
    createprojectwindow.ui \
    mainwindow.ui \
    projectsettingwindow.ui

TRANSLATIONS += \
    Sensors_CPP_en_001.ts

# Default rules for deployment.
target.path = $$[PWD]
!isEmpty(target.path): INSTALLS += target
