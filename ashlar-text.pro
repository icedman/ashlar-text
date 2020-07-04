TARGET = ashlar

QT += widgets svg webkitwidgets network xml core
requires(qtConfig(filedialog))

HEADERS         = src/commands.h \
                  src/editor.h \
                  src/extension.h \
                  src/gutter.h \
                  src/highlighter.h \
                  src/icons.h \
                  src/js.h \
                  src/mainwindow.h \
                  src/minimap.h \
                  src/sidebar.h \
                  src/settings.h \
                  src/select.h \
                  src/tabs.h \
                  src/tmedit.h \
                  src/process.h \
                  ./js-qt/qt/core.h \
                  ./js-qt/qt/engine.h \
                  ./easing/PennerEasing/Cubic.h

SOURCES         = src/commands.cpp \
                  src/editor.cpp \
                  src/extension.cpp \
                  src/gutter.cpp \
                  src/highlighter.cpp \
                  src/icons.cpp \
                  src/js.cpp \
                  src/mainwindow.cpp \
                  src/minimap.cpp \
                  src/sidebar.cpp \
                  src/select.cpp \
                  src/settings.cpp \
                  src/tabs.cpp \
                  src/tmedit.cpp \
                  src/process.cpp \
                  src/main.cpp \
                  ./js-qt/qt/core.cpp \
                  ./js-qt/qt/engine.cpp \
                  ./easing/PennerEasing/Cubic.cpp \
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

RESOURCES += resources.qrc
                  
QMAKE_CXXFLAGS += -fpermissive
CONFIG += c++17

INCPATH +=  ./src
INCPATH +=  ./easing/PennerEasing
INCPATH +=  ./js-qt
INCPATH +=  ./tm-parser/textmate/parser
INCPATH +=  ./tm-parser/textmate/scopes
INCPATH +=  ./tm-parser/textmate/theme

INCPATH += ./tm-parser/subprojects/jsoncpp-1.8.4/include 
LIBS+= ./tm-parser/build/subprojects/jsoncpp-1.8.4/libjsoncpp.a

INCPATH += ./Onigmo
LIBS+= ./Onigmo/.libs/libonigmo.a

resources.path += /usr/local/share/ashlar
resources.files += ./resources/* \
        ./css/* \
        ./extensions
        
target.path = /usr/local/bin/

INSTALLS += target \
            resources