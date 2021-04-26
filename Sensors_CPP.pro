QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    actuator.cpp \
    actuatoraudio.cpp \
    animation.cpp \
    animationaudio.cpp \
    createproject.cpp \
    frame.cpp \
    helper.cpp \
    main.cpp \
    mainwindow.cpp \
    paintingwidget.cpp \
    playinganimation.cpp \
    projectsetting.cpp

HEADERS += \
    Windows.h \
    actuator.h \
    actuatoraudio.h \
    animation.h \
    animationaudio.h \
    createproject.h \
    frame.h \
    helper.h \
    mainwindow.h \
    paintingwidget.h \
    playinganimation.h \
    projectsetting.h

FORMS += \
    createproject.ui \
    mainwindow.ui \
    projectsetting.ui

TRANSLATIONS += \
    Sensors_CPP_en_001.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
