QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += multimedia

CONFIG += c++20 static

QMAKE_CXX_FLAGS += /EHa

INCLUDEPATH += $$PWD/src
INCLUDEPATH += $$PWD/include
INCLUDEPATH += $$PWD/ui

SOURCES += \
    src/animation.cpp \
    src/animationaudio.cpp \
    src/createprojectwindow.cpp \
    src/frame.cpp \
    src/helper.cpp \
    src/main.cpp \
    src/mainwindow.cpp \
    src/paintingwidget.cpp \
    src/projectsettingwindow.cpp

HEADERS += \
    include/animation.h \
    include/animationaudio.h \
    include/createprojectwindow.h \
    include/frame.h \
    include/helper.h \
    include/mainwindow.h \
    include/paintingwidget.h \
    include/projectsettingwindow.h

FORMS += \
    ui/createprojectwindow.ui \
    ui/mainwindow.ui \
    ui/projectsettingwindow.ui

# Default rules for deployment.
target.path = $$[PWD]
!isEmpty(target.path): INSTALLS += target
