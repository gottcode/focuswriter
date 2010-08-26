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

macx {
	INCLUDEPATH += /Library/Frameworks/hunspell.framework/Headers /Library/Frameworks/libzip.framework/Headers
	LIBS += -framework hunspell -framework libzip
} else:win32 {
	INCLUDEPATH += hunspell libzip
	LIBS += ./hunspell/hunspell1.dll ./libzip/libzip0.dll
} else {
	QMAKE_CXXFLAGS += $$system(pkg-config --cflags hunspell libzip)
	LIBS += $$system(pkg-config --libs hunspell libzip)
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
	src/window.h \
	src/rtf/reader.h \
	src/rtf/tokenizer.h \
	src/rtf/writer.h

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
	src/window.cpp \
	src/rtf/reader.cpp \
	src/rtf/tokenizer.cpp \
	src/rtf/writer.cpp

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

	INSTALLS += target icon desktop icons
}
