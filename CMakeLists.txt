cmake_minimum_required(VERSION 3.16.0)

# Configure project
project(focuswriter VERSION 1.7.6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Core5Compat Gui LinguistTools Network PrintSupport Widgets)
find_package(ZLIB REQUIRED)
if(UNIX AND NOT APPLE)
	include(GNUInstallDirs)

	find_package(PkgConfig REQUIRED)
	pkg_check_modules(Hunspell REQUIRED IMPORTED_TARGET hunspell)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_compile_definitions(
		QT_STRICT_ITERATORS
		QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
		QT_DISABLE_DEPRECATED_BEFORE=0x060000
	)
endif()

# Create translations
file(GLOB translations_SRCS translations/*.ts)
qt6_add_translation(translations_QM ${translations_SRCS})

add_custom_target(update_translations
	COMMAND Qt6::lupdate -no-obsolete -locations none src -ts ${translations_SRCS}
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	VERBATIM
)

# Create program
qt6_add_executable(focuswriter
	src/action_manager.cpp
	src/alert.cpp
	src/alert_layer.cpp
	src/application.cpp
	src/block_stats.cpp
	src/color_button.cpp
	src/deltas.cpp
	src/daily_progress.cpp
	src/daily_progress_dialog.cpp
	src/daily_progress_label.cpp
	src/document.cpp
	src/document_cache.cpp
	src/document_watcher.cpp
	src/document_writer.cpp
	src/find_dialog.cpp
	src/gzip.cpp
	src/image_button.cpp
	src/load_screen.cpp
	src/locale_dialog.cpp
	src/main.cpp
	src/paths.cpp
	src/preferences.cpp
	src/preferences_dialog.cpp
	src/scene_list.cpp
	src/scene_model.cpp
	src/session.cpp
	src/session_manager.cpp
	src/shortcut_edit.cpp
	src/smart_quotes.cpp
	src/sound.cpp
	src/stack.cpp
	src/stats.cpp
	src/symbols_dialog.cpp
	src/symbols_model.cpp
	src/theme.cpp
	src/theme_dialog.cpp
	src/theme_manager.cpp
	src/theme_renderer.cpp
	src/timer.cpp
	src/timer_display.cpp
	src/timer_manager.cpp
	src/utils.cpp
	src/window.cpp
	src/fileformats/docx_reader.cpp
	src/fileformats/docx_writer.cpp
	src/fileformats/format_manager.cpp
	src/fileformats/odt_reader.cpp
	src/fileformats/odt_writer.cpp
	src/fileformats/rtf_reader.cpp
	src/fileformats/rtf_tokenizer.cpp
	src/fileformats/rtf_writer.cpp
	src/fileformats/txt_reader.cpp
	src/qtsingleapplication/qtsingleapplication.cpp
	src/qtsingleapplication/qtlocalpeer.cpp
	src/qtzip/qtzip.cpp
	src/spelling/dictionary_dialog.cpp
	src/spelling/dictionary_manager.cpp
	src/spelling/highlighter.cpp
	src/spelling/spell_checker.cpp
	resources/images/images.qrc
	${translations_QM}
)

target_include_directories(focuswriter PRIVATE
	src
	src/fileformats
	src/qtsingleapplication
	src/qtzip
	src/spelling
)

target_link_libraries(focuswriter PRIVATE
	Qt6::Concurrent
	Qt6::Core
	Qt6::Core5Compat
	Qt6::Gui
	Qt6::Network
	Qt6::PrintSupport
	Qt6::Widgets
	ZLIB::ZLIB
)

target_compile_definitions(focuswriter PRIVATE VERSIONSTR="${PROJECT_VERSION}")

if(UNIX AND NOT APPLE)
	target_compile_definitions(focuswriter PRIVATE DATADIR="${CMAKE_INSTALL_DATADIR}")

	target_sources(focuswriter PRIVATE
		src/spelling/dictionary_provider_hunspell.cpp
		src/spelling/dictionary_provider_voikko.cpp
		resources/images/icons/icons.qrc
	)

	target_link_libraries(focuswriter PRIVATE PkgConfig::Hunspell)
elseif(WIN32)
	target_compile_definitions(focuswriter PRIVATE HUNSPELL_STATIC)

	target_include_directories(focuswriter PRIVATE src/spelling/hunspell)

	target_sources(focuswriter PRIVATE
		src/spelling/dictionary_provider_hunspell.cpp
		src/spelling/dictionary_provider_voikko.cpp
		src/spelling/hunspell/affentry.cxx
		src/spelling/hunspell/affixmgr.cxx
		src/spelling/hunspell/csutil.cxx
		src/spelling/hunspell/filemgr.cxx
		src/spelling/hunspell/hashmgr.cxx
		src/spelling/hunspell/hunspell.cxx
		src/spelling/hunspell/hunzip.cxx
		src/spelling/hunspell/phonet.cxx
		src/spelling/hunspell/replist.cxx
		src/spelling/hunspell/suggestmgr.cxx
	)
endif()

# Create symbols
add_subdirectory(resources/symbols)

# Install
if(UNIX AND NOT APPLE)
	install(TARGETS focuswriter RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(FILES ${translations_QM} DESTINATION ${CMAKE_INSTALL_DATADIR}/focuswriter/translations)
	install(DIRECTORY resources/images/icons/hicolor DESTINATION ${CMAKE_INSTALL_DATADIR}/icons)
	install(DIRECTORY resources/images/icons/oxygen/hicolor DESTINATION ${CMAKE_INSTALL_DATADIR}/focuswriter/icons)
	install(DIRECTORY resources/sounds DESTINATION ${CMAKE_INSTALL_DATADIR}/focuswriter)
	install(DIRECTORY resources/themes DESTINATION ${CMAKE_INSTALL_DATADIR}/focuswriter)
	install(FILES resources/symbols/symbols1300.dat DESTINATION ${CMAKE_INSTALL_DATADIR}/focuswriter)
	install(FILES resources/unix/focuswriter.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 COMPONENT doc)
	install(FILES resources/unix/focuswriter.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
	install(FILES resources/unix/focuswriter.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
elseif(WIN32)
	# Use Qt6 macro until CMake provides something
	# https://bugreports.qt.io/browse/QTBUG-87618
	set_target_properties(focuswriter PROPERTIES
		WIN32_EXECUTABLE TRUE
		QT_TARGET_VERSION "${PROJECT_VERSION}"
		QT_TARGET_COMPANY_NAME "Graeme Gott"
		QT_TARGET_DESCRIPTION "Fullscreen word processor"
		QT_TARGET_COPYRIGHT "Copyright (C) 2021 Graeme Gott"
		QT_TARGET_PRODUCT_NAME "FocusWriter"
		QT_TARGET_RC_ICONS ${CMAKE_SOURCE_DIR}/resources/windows/focuswriter.ico
	)
	_qt_internal_generate_win32_rc_file(focuswriter)
endif()
