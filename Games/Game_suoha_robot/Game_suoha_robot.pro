QT       -= core gui
CONFIG   += plugin c++11

TARGET = Game_Suoha_android
TEMPLATE = lib

DESTDIR = ../../../GameServer/
DEFINES += GAME_SUOHA_ANDROID_LIBRARY

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_DEPRECATED_WARNINGS

DEFINES += MUDUO_STD_STRING
DEFINES += USE_ONE_THREAD

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += ../..
INCLUDEPATH += ../../proto
INCLUDEPATH += ../../Framework
INCLUDEPATH += ../../Framework/public
INCLUDEPATH += ../../Framework/GameServer
INCLUDEPATH += ../../Framework/GameServer/public
INCLUDEPATH += ../../Framework/thirdpart
INCLUDEPATH += /usr/local/include/google/protobuf
INCLUDEPATH += /usr/local/include/bsoncxx/v_noabi

LIBS += $$PWD/../../Framework/libs/libmuduo.a
LIBS += /usr/local/lib/libprotobuf.a



LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread
LIBS += -pthread -lrt -lm -lz -ldl -lglog


SOURCES += \
    AndroidUserItemSink.cpp \
    ../Game_Suoha/suoha.cpp \
    ../Game_Suoha/cfg.cpp \
    ../Game_Suoha/funcC.cpp \
    ../Game_Suoha/StdRandom.h \
    ../../proto/suoha.Message.pb.cc \

HEADERS += \
    AndroidUserItemSink.h \
    ../Game_Suoha/suoha.h \
    ../Game_Suoha/cfg.h \
    ../Game_Suoha/funcC.h \
    ../../Framework/GameServer/public/IAndroidUserItemSink.h \
    ../../Framework/GameServer/public/IServerUserItem.h \
    ../../Framework/GameServer/public/AndroidUserItem.h \
    ../../proto/suoha.Message.pb.h \

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target
