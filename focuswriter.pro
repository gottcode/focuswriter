TEMPLATE = app
CONFIG += warn_on release
macx {
	# Uncomment the following line to compile on PowerPC Macs
	# QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
	CONFIG += x86 ppc
}

MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build

unix: !macx {
	TARGET = focuswriter
} else {
	TARGET = FocusWriter
}

# Include hunspell
isEmpty(USE_SYSTEM_HUNSPELL) {
	unix: !macx {
		USE_SYSTEM_HUNSPELL = "yes"
	}
} else {
	contains(USE_SYSTEM_HUNSPELL, "no") {
		USE_SYSTEM_HUNSPELL = ""
	}
}
isEmpty(USE_SYSTEM_HUNSPELL) {
	QMAKE_CXXFLAGS += -Ihunspell
	win32 {
		QMAKE_CXXFLAGS += -DHUNSPELL_STATIC
	}
	SOURCES += hunspell/affentry.cxx \
		hunspell/affixmgr.cxx \
		hunspell/csutil.cxx \
		hunspell/filemgr.cxx \
		hunspell/hashmgr.cxx \
		hunspell/hunspell.cxx \
		hunspell/hunzip.cxx \
		hunspell/phonet.cxx \
		hunspell/replist.cxx \
		hunspell/suggestmgr.cxx
} else {
	QMAKE_CXXFLAGS += $$system(pkg-config --cflags hunspell)
	LIBS += $$system(pkg-config --libs hunspell)
}

# Include minizip
SOURCES += minizip/ioapi.c \
	minizip/unzip.c \
	minizip/zip.c
macx {
	LIBS += -lz
}

HEADERS += src/alert.h \
	src/alert_layer.h \
	src/color_button.h \
	src/deltas.h \
	src/dictionary.h \
	src/document.h \
	src/find_dialog.h \
	src/highlighter.h \
	src/image_button.h \
	src/load_screen.h \
	src/preferences.h \
	src/preferences_dialog.h \
	src/session.h \
	src/session_manager.h \
	src/spell_checker.h \
	src/stack.h \
	src/theme.h \
	src/theme_dialog.h \
	src/theme_manager.h \
	src/timer.h \
	src/timer_display.h \
	src/timer_manager.h \
	src/window.h

SOURCES += src/alert.cpp \
	src/alert_layer.cpp \
	src/color_button.cpp \
	src/deltas.cpp \
	src/dictionary.cpp \
	src/document.cpp \
	src/find_dialog.cpp \
	src/highlighter.cpp \
	src/image_button.cpp \
	src/load_screen.cpp \
	src/main.cpp \
	src/preferences.cpp \
	src/preferences_dialog.cpp \
	src/session.cpp \
	src/session_manager.cpp \
	src/spell_checker.cpp \
	src/stack.cpp \
	src/theme.cpp \
	src/theme_dialog.cpp \
	src/theme_manager.cpp \
	src/timer.cpp \
	src/timer_display.cpp \
	src/timer_manager.cpp \
	src/window.cpp

TRANSLATIONS = translations/focuswriter_fr.ts translations/focuswriter_pt.ts

RESOURCES = icons/icons.qrc translations/translations.qrc
macx {
	ICON = icons/focuswriter.icns
}
win32 {
	RC_FILE = icons/icon.rc
}

unix: !macx {
	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	target.path = $$PREFIX/bin/

	icon.files = icons/focuswriter.png
	icon.path = $$PREFIX/share/icons/hicolor/48x48/apps

	icons.files = icons/oxygen/hicolor/*
	icons.path = $$PREFIX/share/focuswriter/icons/hicolor

	desktop.files = icons/focuswriter.desktop
	desktop.path = $$PREFIX/share/applications/

	isEmpty(USE_SYSTEM_HUNSPELL) {
		!exists( $$PREFIX/share/hunspell/en_US.aff ) {
			dictionary.files = hunspell/en_US/*
			dictionary.path = $$PREFIX/share/hunspell/
			INSTALLS += dictionary
		}
	}

	INSTALLS += target icon desktop icons
}
