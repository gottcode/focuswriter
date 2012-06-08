TEMPLATE = app
CONFIG += warn_on release
macx {
	QMAKE_INFO_PLIST = resources/mac/Info.plist
	CONFIG += x86_64
}

!win32 {
	LIBS += -lz
}

MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build

VERSION = $$system(git rev-parse --short HEAD)
isEmpty(VERSION) {
	VERSION = 0
}
DEFINES += VERSIONSTR=\\\"git.$${VERSION}\\\"

unix: !macx {
	TARGET = focuswriter
} else {
	TARGET = FocusWriter
}

macx {
	INCLUDEPATH += src/nsspellchecker /Library/Frameworks/libzip.framework/Headers
	LIBS += -framework libzip -framework AppKit

	HEADERS += src/rtf/clipboard_mac.h \
		src/nsspellchecker/dictionary.h \
		src/nsspellchecker/dictionary_data.h \
		src/nsspellchecker/dictionary_manager.h

	SOURCES += src/rtf/clipboard_mac.cpp

	OBJECTIVE_SOURCES += src/nsspellchecker/dictionary.mm \
		src/nsspellchecker/dictionary_data.mm \
		src/nsspellchecker/dictionary_manager.mm
} else:win32 {
	INCLUDEPATH += enchant libzip src/enchant
	LIBS += ./enchant/libenchant.dll ./libzip/libzip0.dll -lOle32

	HEADERS += src/rtf/clipboard_windows.h \
		src/enchant/dictionary.h \
		src/enchant/dictionary_data.h \
		src/enchant/dictionary_manager.h

	SOURCES += src/rtf/clipboard_windows.cpp \
		src/enchant/dictionary.cpp \
		src/enchant/dictionary_data.cpp \
		src/enchant/dictionary_manager.cpp
} else {
	INCLUDEPATH += src/enchant

	CONFIG += link_pkgconfig
	PKGCONFIG += enchant libzip

	HEADERS += src/enchant/dictionary.h \
		src/enchant/dictionary_data.h \
		src/enchant/dictionary_manager.h

	SOURCES += src/enchant/dictionary.cpp \
		src/enchant/dictionary_data.cpp \
		src/enchant/dictionary_manager.cpp
}

HEADERS += src/alert.h \
	src/alert_layer.h \
	src/block_stats.h \
	src/color_button.h \
	src/deltas.h \
	src/document.h \
	src/find_dialog.h \
	src/gzip.h \
	src/highlighter.h \
	src/image_button.h \
	src/load_screen.h \
	src/locale_dialog.h \
	src/odt_reader.h \
	src/preferences.h \
	src/preferences_dialog.h \
	src/session.h \
	src/session_manager.h \
	src/settings_file.h \
	src/smart_quotes.h \
	src/sound.h \
	src/spell_checker.h \
	src/stack.h \
	src/stats.h \
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
	src/block_stats.cpp \
	src/color_button.cpp \
	src/deltas.cpp \
	src/document.cpp \
	src/find_dialog.cpp \
	src/gzip.cpp \
	src/highlighter.cpp \
	src/image_button.cpp \
	src/load_screen.cpp \
	src/locale_dialog.cpp \
	src/main.cpp \
	src/odt_reader.cpp \
	src/preferences.cpp \
	src/preferences_dialog.cpp \
	src/session.cpp \
	src/session_manager.cpp \
	src/smart_quotes.cpp \
	src/sound.cpp \
	src/spell_checker.cpp \
	src/stack.cpp \
	src/stats.cpp \
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

TRANSLATIONS = translations/focuswriter_cs.ts \
	translations/focuswriter_da.ts \
	translations/focuswriter_de.ts \
	translations/focuswriter_el.ts \
	translations/focuswriter_en.ts \
	translations/focuswriter_es.ts \
	translations/focuswriter_es_MX.ts \
	translations/focuswriter_fi.ts \
	translations/focuswriter_fr.ts \
	translations/focuswriter_hu.ts \
	translations/focuswriter_it.ts \
	translations/focuswriter_nl.ts \
	translations/focuswriter_pl.ts \
	translations/focuswriter_pt.ts \
	translations/focuswriter_pt_BR.ts \
	translations/focuswriter_ru.ts \
	translations/focuswriter_sv.ts \
	translations/focuswriter_uk.ts

RESOURCES = resources/images/images.qrc resources/images/icons/icons.qrc
macx {
	ICON = resources/mac/focuswriter.icns
}
win32 {
	RC_FILE = resources/windows/icon.rc
}

macx {
	ICONS.files = resources/images/icons/oxygen-icons/oxygen/
	ICONS.path = Contents/Resources/icons/oxygen

	SOUNDS.files = resources/sounds
	SOUNDS.path = Contents/Resources

	QMAKE_BUNDLE_DATA += ICONS SOUNDS
}

unix: !macx {
	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}

	target.path = $$PREFIX/bin/

	icon.files = resources/images/icons/hicolor/*
	icon.path = $$PREFIX/share/icons/hicolor/

	pixmap.files = resources/unix/focuswriter_32.xpm
	pixmap.path = $$PREFIX/share/pixmaps/

	icons.files = resources/images/icons/oxygen-icons/oxygen/*
	icons.path = $$PREFIX/share/focuswriter/icons/oxygen

	desktop.files = resources/unix/focuswriter.desktop
	desktop.path = $$PREFIX/share/applications/

	qm.files = translations/*.qm
	qm.path = $$PREFIX/share/focuswriter/translations

	sounds.files = resources/sounds/*
	sounds.path = $$PREFIX/share/focuswriter/sounds

	INSTALLS += target icon pixmap desktop icons qm sounds
}
