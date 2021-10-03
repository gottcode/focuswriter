!versionAtLeast(QT_VERSION, 5.12) {
	error("FocusWriter requires Qt 5.12 or greater")
}

TEMPLATE = app
QT += network widgets printsupport concurrent
greaterThan(QT_MAJOR_VERSION, 5) {
	QT += core5compat
}
lessThan(QT_MAJOR_VERSION, 6) {
	QT += multimedia
	macx {
		QT += macextras
	} else:win32 {
		QT += winextras
	}
}
macx {
	QMAKE_INFO_PLIST = resources/mac/Info.plist
}
CONFIG += c++17

CONFIG(debug, debug|release) {
	CONFIG += warn_on
	DEFINES += QT_DEPRECATED_WARNINGS
	DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000
	DEFINES += QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
}

# Allow in-tree builds
MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build

# Set program version
VERSION = 1.7.0
VERSIONSTR = $$system(git describe)
isEmpty(VERSIONSTR) {
	VERSIONSTR = $$VERSION
}
DEFINES += VERSIONSTR=\\\"$${VERSIONSTR}\\\"

# Set program name
unix: !macx: !haiku {
	TARGET = focuswriter
} else {
	TARGET = FocusWriter
}

# Add dependencies
macx {
	lessThan(QT_MAJOR_VERSION, 6) {
		DEFINES += RTFCLIPBOARD
		HEADERS += src/fileformats/clipboard_mac.h
		SOURCES += src/fileformats/clipboard_mac.cpp
	}

	LIBS += -lz -framework AppKit

	HEADERS += src/spelling/dictionary_provider_nsspellchecker.h
	OBJECTIVE_SOURCES += src/spelling/dictionary_provider_nsspellchecker.mm
} else:win32 {
	lessThan(QT_MAJOR_VERSION, 6) {
		DEFINES += RTFCLIPBOARD
		HEADERS += src/fileformats/clipboard_windows.h
		SOURCES += src/fileformats/clipboard_windows.cpp
	}

	DEFINES += HUNSPELL_STATIC

	LIBS += -lz -lole32

	INCLUDEPATH += src/spelling/hunspell

	HEADERS += src/spelling/dictionary_provider_hunspell.h \
		src/spelling/dictionary_provider_voikko.h

	SOURCES += src/spelling/dictionary_provider_hunspell.cpp \
		src/spelling/dictionary_provider_voikko.cpp \
		src/spelling/hunspell/affentry.cxx \
		src/spelling/hunspell/affixmgr.cxx \
		src/spelling/hunspell/csutil.cxx \
		src/spelling/hunspell/filemgr.cxx \
		src/spelling/hunspell/hashmgr.cxx \
		src/spelling/hunspell/hunspell.cxx \
		src/spelling/hunspell/hunzip.cxx \
		src/spelling/hunspell/phonet.cxx \
		src/spelling/hunspell/replist.cxx \
		src/spelling/hunspell/suggestmgr.cxx
} else:unix {
	CONFIG += link_pkgconfig
	PKGCONFIG += hunspell zlib

	HEADERS += src/spelling/dictionary_provider_hunspell.h \
		src/spelling/dictionary_provider_voikko.h

	SOURCES += src/spelling/dictionary_provider_hunspell.cpp \
		src/spelling/dictionary_provider_voikko.cpp
}

INCLUDEPATH += src src/fileformats src/qtsingleapplication src/qtzip src/spelling

# Specify program sources
HEADERS += src/action_manager.h \
	src/alert.h \
	src/alert_layer.h \
	src/application.h \
	src/block_stats.h \
	src/color_button.h \
	src/deltas.h \
	src/daily_progress.h \
	src/daily_progress_dialog.h \
	src/daily_progress_label.h \
	src/document.h \
	src/document_cache.h \
	src/document_watcher.h \
	src/document_writer.h \
	src/find_dialog.h \
	src/gzip.h \
	src/image_button.h \
	src/load_screen.h \
	src/locale_dialog.h \
	src/paths.h \
	src/preferences.h \
	src/preferences_dialog.h \
	src/ranged_int.h \
	src/ranged_string.h \
	src/scene_list.h \
	src/scene_model.h \
	src/session.h \
	src/session_manager.h \
	src/settings_file.h \
	src/shortcut_edit.h \
	src/smart_quotes.h \
	src/sound.h \
	src/stack.h \
	src/stats.h \
	src/symbols_dialog.h \
	src/symbols_model.h \
	src/theme.h \
	src/theme_dialog.h \
	src/theme_manager.h \
	src/theme_renderer.h \
	src/timer.h \
	src/timer_display.h \
	src/timer_manager.h \
	src/utils.h \
	src/window.h \
	src/word_ref.h \
	src/fileformats/docx_reader.h \
	src/fileformats/docx_writer.h \
	src/fileformats/format_manager.h \
	src/fileformats/format_reader.h \
	src/fileformats/odt_reader.h \
	src/fileformats/odt_writer.h \
	src/fileformats/rtf_reader.h \
	src/fileformats/rtf_tokenizer.h \
	src/fileformats/rtf_writer.h \
	src/fileformats/txt_reader.h \
	src/qtsingleapplication/qtsingleapplication.h \
	src/qtsingleapplication/qtlocalpeer.h \
	src/qtzip/qtzipreader.h \
	src/qtzip/qtzipwriter.h \
	src/spelling/abstract_dictionary.h \
	src/spelling/abstract_dictionary_provider.h \
	src/spelling/dictionary_dialog.h \
	src/spelling/dictionary_manager.h \
	src/spelling/dictionary_ref.h \
	src/spelling/highlighter.h \
	src/spelling/spell_checker.h

SOURCES += src/action_manager.cpp \
	src/alert.cpp \
	src/alert_layer.cpp \
	src/application.cpp \
	src/block_stats.cpp \
	src/color_button.cpp \
	src/deltas.cpp \
	src/daily_progress.cpp \
	src/daily_progress_dialog.cpp \
	src/daily_progress_label.cpp \
	src/document.cpp \
	src/document_cache.cpp \
	src/document_watcher.cpp \
	src/document_writer.cpp \
	src/find_dialog.cpp \
	src/gzip.cpp \
	src/image_button.cpp \
	src/load_screen.cpp \
	src/locale_dialog.cpp \
	src/main.cpp \
	src/paths.cpp \
	src/preferences.cpp \
	src/preferences_dialog.cpp \
	src/scene_list.cpp \
	src/scene_model.cpp \
	src/session.cpp \
	src/session_manager.cpp \
	src/shortcut_edit.cpp \
	src/smart_quotes.cpp \
	src/sound.cpp \
	src/stack.cpp \
	src/stats.cpp \
	src/symbols_dialog.cpp \
	src/symbols_model.cpp \
	src/theme.cpp \
	src/theme_dialog.cpp \
	src/theme_manager.cpp \
	src/theme_renderer.cpp \
	src/timer.cpp \
	src/timer_display.cpp \
	src/timer_manager.cpp \
	src/utils.cpp \
	src/window.cpp \
	src/fileformats/docx_reader.cpp \
	src/fileformats/docx_writer.cpp \
	src/fileformats/format_manager.cpp \
	src/fileformats/odt_reader.cpp \
	src/fileformats/odt_writer.cpp \
	src/fileformats/rtf_reader.cpp \
	src/fileformats/rtf_tokenizer.cpp \
	src/fileformats/rtf_writer.cpp \
	src/fileformats/txt_reader.cpp \
	src/qtsingleapplication/qtsingleapplication.cpp \
	src/qtsingleapplication/qtlocalpeer.cpp \
	src/qtzip/qtzip.cpp \
	src/spelling/dictionary_dialog.cpp \
	src/spelling/dictionary_manager.cpp \
	src/spelling/highlighter.cpp \
	src/spelling/spell_checker.cpp

# Generate translations
TRANSLATIONS = $$files(translations/focuswriter_*.ts)
qtPrepareTool(LRELEASE, lrelease)
updateqm.input = TRANSLATIONS
updateqm.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$LRELEASE -silent ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += updateqm

# Install program data
RESOURCES = resources/images/images.qrc

macx {
	ICON = resources/mac/focuswriter.icns

	ICONS.files = resources/images/icons/oxygen/hicolor
	ICONS.path = Contents/Resources/icons

	SOUNDS.files = resources/sounds
	SOUNDS.path = Contents/Resources

	SYMBOLS.files = resources/symbols/symbols1400.dat
	SYMBOLS.path = Contents/Resources

	THEMES.files = resources/themes
	THEMES.path = Contents/Resources

	QMAKE_BUNDLE_DATA += ICONS SOUNDS SYMBOLS THEMES
} else:win32 {
	RC_ICONS = resources/windows/focuswriter.ico
	QMAKE_TARGET_DESCRIPTION = "Fullscreen word processor"
	QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2021 Graeme Gott"
} else:haiku {
	RESOURCES += resources/images/icons/icons.qrc

	isEmpty(PREFIX) {
		PREFIX = /boot/home/config/non-packaged/apps/FocusWriter
	}
	isEmpty(BINDIR) {
		BINDIR = $$PREFIX
	}
	isEmpty(DATADIR) {
		DATADIR = $$PREFIX/data
	}
	DEFINES += DATADIR=\\\"$$DATADIR\\\"
	target.path = $$BINDIR

	icons.files = resources/images/icons/oxygen/hicolor/*
	icons.path = $$DATADIR/icons/hicolor

	man.files = resources/unix/focuswriter.1
	man.path = $$PREFIX/../../documentation/man/man1

	qm.files = $$replace(TRANSLATIONS, .ts, .qm)
	qm.path = $$DATADIR/translations
	qm.CONFIG += no_check_exist

	sounds.files = resources/sounds/*
	sounds.path = $$DATADIR/sounds

	themes.files = resources/themes/*
	themes.path = $$DATADIR/themes

	symbols.files = resources/symbols/symbols1400.dat
	symbols.path = $$DATADIR

	INSTALLS += target man icons qm sounds symbols themes
} else:unix {
	RESOURCES += resources/images/icons/icons.qrc

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}
	isEmpty(BINDIR) {
		BINDIR = $$PREFIX/bin
	}
	isEmpty(DATADIR) {
		DATADIR = $$PREFIX/share
	}
	DEFINES += DATADIR=\\\"$${DATADIR}/focuswriter\\\"

	target.path = $$BINDIR

	icon.files = resources/images/icons/hicolor/*
	icon.path = $$DATADIR/icons/hicolor

	icons.files = resources/images/icons/oxygen/hicolor/*
	icons.path = $$DATADIR/focuswriter/icons/hicolor

	desktop.files = resources/unix/focuswriter.desktop
	desktop.path = $$DATADIR/applications/

	appdata.files = resources/unix/focuswriter.appdata.xml
	appdata.path = $$DATADIR/metainfo/

	man.files = resources/unix/focuswriter.1
	man.path = $$PREFIX/share/man/man1

	qm.files = $$replace(TRANSLATIONS, .ts, .qm)
	qm.path = $$DATADIR/focuswriter/translations
	qm.CONFIG += no_check_exist

	sounds.files = resources/sounds/*
	sounds.path = $$DATADIR/focuswriter/sounds

	themes.files = resources/themes/*
	themes.path = $$DATADIR/focuswriter/themes

	symbols.files = resources/symbols/symbols1400.dat
	symbols.path = $$DATADIR/focuswriter

	INSTALLS += target icon desktop appdata man icons qm sounds symbols themes
}
