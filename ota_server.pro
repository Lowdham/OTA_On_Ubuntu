QT -= gui

QT +=network

CONFIG += c++17 console
CONFIG -= app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        otalib/bsdiff/bsdiff.c \
        otalib/bsdiff/bspatch.c \
        otalib/delta_log.cpp \
        otalib/diff.cpp \
        otalib/signature.cpp \
        otalib/ssl_socket_client.cpp \
        server/src/InetAddress.cc \
        server/src/SSL.cc \
        server/src/ServerSocket.cc \
        server/src/TcpConnection.cc \
        server/src/TcpServer.cc \
        server/src/TimerQueue.cc \
        server/src/server.cpp \
        server_main.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    otalib/bsdiff/bsdiff.h \
    otalib/bsdiff/bspatch.h \
    otalib/buffer.hpp \
    otalib/delta_log.h \
    otalib/diff.h \
    otalib/file_logger.h \
    otalib/logger/logger.h \
    otalib/sha256_hash.h \
    otalib/merklecpp.h \
    otalib/otaerr.hpp \
    otalib/pack_apply.hpp \
    otalib/shell_cmd.hpp \
    otalib/signature.h \
    otalib/ssl_socket_client.hpp \
    otalib/update_strategy.hpp \
    otalib/utils.hpp \
    otalib/vcm.hpp \
    otalib/version.hpp \
    server/include/Buffer.h \
    server/include/DirectoryWatcher.h \
    server/include/FileLoader.hpp \
    server/include/InetAddress.h \
    server/include/SSL.h \
    server/include/ServerSocket.h \
    server/include/TcpConnection.h \
    server/include/TcpServer.h \
    server/include/ThreadPool.h \
    server/include/TimerHeap.h \
    server/include/TimerQueue.h \
    server/include/server.h \
    server/include/timestamp.h \

LIBS += -lssl -lcrypto -lpthread

TEMPLATE = app
TARGET = bin/otaserver
MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build
