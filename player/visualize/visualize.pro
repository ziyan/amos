QT += network opengl sql
CONFIG += no_keywords debug
TARGET = visualize
TEMPLATE = app
QMAKE_CLEAN += */*~

MOC_DIR = .moc
OBJECTS_DIR = .obj
UI_DIR = .ui

INCLUDEPATH += /usr/local/include/player-3.0
LIBS += -lplayerc++ -lboost_thread-mt -lboost_signals-mt -lplayerc -lm -lz -lplayerinterface -lplayerwkb -lplayercommon
INCLUDEPATH += ../common
LIBS += -lmap -L../.build/lib -lmemcached

DEPENDPATH += ui/ model/

SOURCES += main.cpp \
	ui/glwidget.cpp ui/glinfo.cpp \
	model/robot.cpp model/world.cpp
HEADERS += ui/glwidget.h ui/glinfo.h \
	model/robot.h model/world.h
FORMS +=
