
QT       -= core gui
CONFIG   += plugin c++11

TARGET = Game_S13s
TEMPLATE = lib

DESTDIR = ../../../GameServer/
DEFINES += GAME_S13S_LIBRARY

DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += MUDUO_STD_STRING

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

LIBS += $$PWD/../../Framework/libs/libmuduo.a
LIBS += /usr/local/lib/libprotobuf.a


INCLUDEPATH += /usr/local/include/bsoncxx/v_noabi



LIBS += -lboost_date_time -lboost_filesystem -lboost_system -lboost_thread
LIBS += -lfreetype -ljpeg -lpng -lgd -pthread -lrt -lm -lz -ldl -lglog



SOURCES += \
    TableFrameSink.cpp \
    s13s.cpp \
    cfg.cpp \
    funcC.cpp \
    ../../Framework/public/GlobalFunc.cpp \
    ../../proto/Game.Common.pb.cc \
    ../../proto/GameServer.Message.pb.cc \
    ../../proto/s13s.Message.pb.cc \

HEADERS += \
    TableFrameSink.h \
    s13s.h \
    cfg.h \
    funcC.h \
    StdRandom.h \
    AnimatePlay.h \
    ../../Framework/public/GlobalFunc.h \
    ../../Framework/public/ToolTime.h \
    ../../Framework/GameServer/public/Globals.h \
    ../../Framework/GameServer/public/gameDefine.h \
    ../../Framework/GameServer/public/ITableFrame.h \
    ../../Framework/GameServer/public/ITableFrameSink.h \
    ../../Framework/GameServer/public/IAndroidUserItemSink.h \
	../../Framework/GameServer/GameServerDefine.h \
    ../../Framework/GameServer/ServerUserItem.h \
    ../../proto/Game.Common.pb.h \
    ../../proto/GameServer.Message.pb.h \
    ../../proto/s13s.Message.pb.h \

# Default rules for deployment.
unix {
    target.path = /usr/lib
    INSTALLS += target
}

DISTFILES += \
    ../../proto/src/s13s.Message.proto
