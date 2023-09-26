QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += multimedia

CONFIG += c++20 static

QMAKE_CXX_FLAGS += /EHa

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

# Default rules for deployment.
target.path = $$[PWD]
!isEmpty(target.path): INSTALLS += target
