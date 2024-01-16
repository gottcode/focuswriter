# SPDX-FileCopyrightText: 2021-2023 Graeme Gott <graeme@gottcode.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

# Configure project
project(focuswriter VERSION 1.8.6 LANGUAGES CXX)

set(project_copyright "2008-2024 Graeme Gott")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if (WIN32)
	set(Iconv_IS_BUILT_IN true)
endif()

find_package(Qt6 REQUIRED COMPONENTS Concurrent Core Gui LinguistTools Multimedia Network PrintSupport Widgets)
find_package(Iconv REQUIRED)
find_package(ZLIB REQUIRED)
include(GNUInstallDirs)

add_compile_definitions(
	QT_NO_KEYWORDS
	$<$<CONFIG:DEBUG>:QT_STRICT_ITERATORS>
	$<$<CONFIG:DEBUG>:QT_NO_NARROWING_CONVERSIONS_IN_CONNECT>
	$<$<CONFIG:DEBUG>:QT_DISABLE_DEPRECATED_BEFORE=0x060600>
)

# Version number
include(cmake/AddVersionCompileDefinition.cmake)
add_version_compile_definition(src/application.cpp VERSIONSTR)

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
	src/text_codec.cpp
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
	src/fileformats/html_writer.cpp
	src/fileformats/odt_reader.cpp
	src/fileformats/odt_writer.cpp
	src/fileformats/rtf_reader.cpp
	src/fileformats/rtf_tokenizer.cpp
	src/fileformats/rtf_writer.cpp
	src/fileformats/txt_reader.cpp
	src/spelling/dictionary_dialog.cpp
	src/spelling/dictionary_manager.cpp
	src/spelling/highlighter.cpp
	src/spelling/spell_checker.cpp
	src/3rdparty/qtsingleapplication/qtsingleapplication.cpp
	src/3rdparty/qtsingleapplication/qtlocalpeer.cpp
	src/3rdparty/qtzip/qtzip.cpp
	resources/images/images.qrc
	${translations_QM}
)

target_include_directories(focuswriter PRIVATE
	src
	src/fileformats
	src/spelling
	src/3rdparty/qtsingleapplication
	src/3rdparty/qtzip
)

target_link_libraries(focuswriter PRIVATE
	Qt6::Concurrent
	Qt6::Core
	Qt6::Gui
	Qt6::Multimedia
	Qt6::Network
	Qt6::PrintSupport
	Qt6::Widgets
	Iconv::Iconv
	ZLIB::ZLIB
)

# Add spellchecking support
if(APPLE)
	target_sources(focuswriter PRIVATE
		src/spelling/dictionary_provider_nsspellchecker.mm
	)

	find_library(APPKIT AppKit)
	target_link_libraries(focuswriter PRIVATE ${APPKIT})
elseif(WIN32)
	target_compile_definitions(focuswriter PRIVATE HUNSPELL_STATIC)

	target_include_directories(focuswriter PRIVATE src/3rdparty/hunspell)

	target_sources(focuswriter PRIVATE
		src/spelling/dictionary_provider_hunspell.cpp
		src/spelling/dictionary_provider_voikko.cpp
		src/3rdparty/hunspell/affentry.cxx
		src/3rdparty/hunspell/affixmgr.cxx
		src/3rdparty/hunspell/csutil.cxx
		src/3rdparty/hunspell/filemgr.cxx
		src/3rdparty/hunspell/hashmgr.cxx
		src/3rdparty/hunspell/hunspell.cxx
		src/3rdparty/hunspell/hunzip.cxx
		src/3rdparty/hunspell/phonet.cxx
		src/3rdparty/hunspell/replist.cxx
		src/3rdparty/hunspell/suggestmgr.cxx
	)
else()
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(Hunspell REQUIRED IMPORTED_TARGET hunspell)

	target_sources(focuswriter PRIVATE
		src/spelling/dictionary_provider_hunspell.cpp
		src/spelling/dictionary_provider_voikko.cpp
	)

	target_link_libraries(focuswriter PRIVATE PkgConfig::Hunspell)
endif()

# Optimize build
option(ENABLE_LINK_TIME_OPTIMIZATION "Enable link time optimization" OFF)
if(ENABLE_LINK_TIME_OPTIMIZATION)
	include(CheckIPOSupported)
	check_ipo_supported(RESULT result)
	if(result)
		set_target_properties(focuswriter PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
	endif()
endif()

option(ENABLE_STRIP "Enable automatic stripping of builds" OFF)
if(ENABLE_STRIP)
	add_custom_command(TARGET focuswriter
		POST_BUILD
		COMMAND ${CMAKE_STRIP} $<TARGET_FILE:focuswriter>
	)
endif()

# Create symbols
add_subdirectory(resources/symbols)

# Install
if(APPLE)
	set(datadir "../Resources")

	set_target_properties(focuswriter PROPERTIES
		OUTPUT_NAME FocusWriter
		MACOSX_BUNDLE TRUE
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/resources/mac/Info.plist.in
	)

	include(cmake/BundleResources.cmake)
	bundle_data(focuswriter ${CMAKE_SOURCE_DIR}/resources/mac/focuswriter.icns Resources)
	bundle_data(focuswriter ${CMAKE_SOURCE_DIR}/resources/images/icons/oxygen/hicolor Resources/icons/hicolor)
	bundle_data(focuswriter ${CMAKE_SOURCE_DIR}/resources/sounds Resources/sounds)
	bundle_data(focuswriter ${CMAKE_SOURCE_DIR}/resources/symbols/symbols1510.dat Resources)
	bundle_data(focuswriter ${CMAKE_SOURCE_DIR}/resources/themes Resources/themes)
	bundle_translations(focuswriter "${translations_QM}")
elseif(WIN32)
	set(datadir ".")

	# Use Qt6 macro until CMake provides something
	# https://bugreports.qt.io/browse/QTBUG-87618
	set_target_properties(focuswriter PROPERTIES
		OUTPUT_NAME FocusWriter
		WIN32_EXECUTABLE TRUE
		QT_TARGET_VERSION "${PROJECT_VERSION}"
		QT_TARGET_COMPANY_NAME "Graeme Gott"
		QT_TARGET_DESCRIPTION "Fullscreen word processor"
		QT_TARGET_COPYRIGHT "\\xA9 ${project_copyright}"
		QT_TARGET_PRODUCT_NAME "FocusWriter"
		QT_TARGET_RC_ICONS ${CMAKE_SOURCE_DIR}/resources/windows/focuswriter.ico
	)
	_qt_internal_generate_win32_rc_file(focuswriter)
else()
	file(RELATIVE_PATH datadir ${CMAKE_INSTALL_FULL_BINDIR} ${CMAKE_INSTALL_FULL_DATADIR}/focuswriter)

	target_sources(focuswriter PRIVATE resources/images/icons/icons.qrc)

	install(TARGETS focuswriter RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	install(FILES ${translations_QM} DESTINATION ${CMAKE_INSTALL_DATADIR}/focuswriter/translations)
	install(DIRECTORY resources/images/icons/hicolor DESTINATION ${CMAKE_INSTALL_DATADIR}/icons)
	install(DIRECTORY resources/images/icons/oxygen/hicolor DESTINATION ${CMAKE_INSTALL_DATADIR}/focuswriter/icons)
	install(DIRECTORY resources/sounds DESTINATION ${CMAKE_INSTALL_DATADIR}/focuswriter)
	install(DIRECTORY resources/themes DESTINATION ${CMAKE_INSTALL_DATADIR}/focuswriter)
	install(FILES resources/symbols/symbols1510.dat DESTINATION ${CMAKE_INSTALL_DATADIR}/focuswriter)
	install(FILES resources/unix/focuswriter.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 COMPONENT doc)
	install(FILES resources/unix/focuswriter.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
	install(FILES resources/unix/focuswriter.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
endif()

set_property(SOURCE src/main.cpp APPEND PROPERTY COMPILE_DEFINITIONS FOCUSWRITER_DATADIR="${datadir}")
