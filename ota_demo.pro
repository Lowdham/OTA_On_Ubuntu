QT -= gui
QT += network

CONFIG += c++17 console
CONFIG -= app_bundle
QMAKE_CXXFLAGS += -std=c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        app/control.cpp \
        app/net_module.cpp \
        app/update_module.cpp \
        otalib/bsdiff/bsdiff.c \
        otalib/bsdiff/bspatch.c \
        otalib/delta_log.cpp \
        otalib/diff.cpp \
        otalib/signature.cpp \
        otalib/ssl_socket_client.cpp \
    app.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
  otalib/bsdiff/LICENSE \
  otalib/bsdiff/README.md

HEADERS += \
  app/app_error.hpp \
  app/control.hpp \
  app/net_module.hpp \
  app/properties.hpp \
  app/update_module.hpp \
  otalib/bsdiff/bsdiff.h \
  otalib/bsdiff/bspatch.h \
  otalib/buffer.hpp \
  otalib/delta_log.h \
  otalib/diff.h \
  otalib/file_logger.h \
  otalib/logger/logger.h \
  otalib/logger/logger_color.h \
  otalib/logger/private/logger_color_linux.h \
  otalib/logger/private/logger_color_win.h \
  otalib/merklecpp.h \
  otalib/otaerr.hpp \
  otalib/pack_apply.hpp \
  otalib/property.hpp \
  otalib/sha256_hash.h \
  otalib/shell_cmd.hpp \
  otalib/signature.h \
  otalib/ssl_socket_client.hpp \
  otalib/update_strategy.hpp \
  otalib/utils.hpp \
  otalib/vcm.hpp \
  otalib/version.hpp

LIBS += -lssl -lcrypto -lpthread

TEMPLATE = app
TARGET = bin/app
MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build
