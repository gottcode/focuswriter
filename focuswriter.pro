TEMPLATE = app
VERSION = 1.1.2
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

HEADERS = src/color_button.h \
	src/find_dialog.h \
	src/image_button.h \
	src/image_dialog.h \
	src/preferences.h \
	src/theme.h \
	src/theme_dialog.h \
	src/theme_manager.h \
	src/thumbnail_loader.h \
	src/thumbnail_model.h \
	src/window.h

SOURCES = src/color_button.cpp \
	src/find_dialog.cpp \
	src/image_button.cpp \
	src/image_dialog.cpp \
	src/main.cpp \
	src/preferences.cpp \
	src/theme.cpp \
	src/theme_dialog.cpp \
	src/theme_manager.cpp \
	src/thumbnail_loader.cpp \
	src/thumbnail_model.cpp \
	src/window.cpp

RESOURCES = icons/icons.qrc
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

	desktop.files = icons/focuswriter.desktop
	desktop.path = $$PREFIX/share/applications/

	INSTALLS += target icon desktop
}
