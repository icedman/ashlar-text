TARGET = ashlar

QT += widgets svg webkitwidgets network
requires(qtConfig(filedialog))

HEADERS         = commands.h \
                  editor.h \
                  extension.h \
                  gutter.h \
                  highlighter.h \
                  icons.h \
                  js.h \
                  mainwindow.h \
                  minimap.h \
                  sidebar.h \
                  settings.h \
                  tabs.h \
                  ./js-qt/qt/core.h \
                  ./js-qt/qt/engine.h

SOURCES         = commands.cpp \
                  editor.cpp \
                  extension.cpp \
                  gutter.cpp \
                  highlighter.cpp \
                  icons.cpp \
                  js.cpp \
                  mainwindow.cpp \
                  minimap.cpp \
                  sidebar.cpp \
                  settings.cpp \
                  tabs.cpp \
                  main.cpp \
                  ./js-qt/qt/core.cpp \
                  ./js-qt/qt/engine.cpp \
                  ./tm-parser/textmate/parser/grammar.cpp \
                  ./tm-parser/textmate/parser/reader.cpp \
                  ./tm-parser/textmate/parser/pattern.cpp \
                  ./tm-parser/textmate/parser/parser.cpp \
                  ./tm-parser/textmate/scopes/scope.cpp \
                  ./tm-parser/textmate/scopes/types.cpp \
                  ./tm-parser/textmate/scopes/parse.cpp \
                  ./tm-parser/textmate/scopes/match.cpp \
                  ./tm-parser/textmate/theme/theme.cpp \
                  ./tm-parser/textmate/theme/util.cpp

QMAKE_CXXFLAGS += -fpermissive
CONFIG += c++17

INCPATH +=  ./js-qt
INCPATH +=  ./tm-parser/textmate/parser/
INCPATH +=  ./tm-parser/textmate/scopes/
INCPATH +=  ./tm-parser/textmate/theme/

INCPATH += ./tm-parser/subprojects/jsoncpp-1.8.4/include 
LIBS+= ./tm-parser/build/subprojects/jsoncpp-1.8.4/libjsoncpp.a

INCPATH += ./Onigmo
LIBS+= ./Onigmo/.libs/libonigmo.a
