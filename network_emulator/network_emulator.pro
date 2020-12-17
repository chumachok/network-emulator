QT       += core gui charts network

greaterThan(QT_MAJOR_VERSION, 4): QT += core widgets charts

CONFIG += c++11 console

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

LIBS += -lws2_32

SOURCES += \
    src/main.cpp \
    src/networkemulator.cpp

HEADERS += \
    src/networkemulator.h

FORMS += \
    networkemulator.ui

TRANSLATIONS += \
    network_emulator_en_CA.ts

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
