# SPDX-FileCopyrightText: 2021 Graeme Gott <graeme@gottcode.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.16)

# Configure project
project(focuswriter VERSION 1.9.0 LANGUAGES CXX)

set(project_copyright "2008-2026 Graeme Gott")

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
	$<$<CONFIG:DEBUG>:QT_DISABLE_DEPRECATED_BEFORE=0x061000>
)
add_compile_options(
	$<$<CONFIG:DEBUG>:-Wshadow>
)

# Version number
include(cmake/AddVersionCompileDefinition.cmake)
add_version_compile_definition(src/main.cpp VERSIONSTR)

# Create program
qt_add_executable(focuswriter
	# Headers
	src/action_manager.h
	src/alert.h
	src/alert_layer.h
	src/block_stats.h
	src/color_button.h
	src/deltas.h
	src/daily_progress.h
	src/daily_progress_dialog.h
	src/daily_progress_label.h
	src/document.h
	src/document_cache.h
	src/document_watcher.h
	src/document_writer.h
	src/find_dialog.h
	src/gzip.h
	src/image_button.h
	src/load_screen.h
	src/locale_dialog.h
	src/paths.h
	src/preferences.h
	src/preferences_dialog.h
	src/scene_list.h
	src/scene_model.h
	src/session.h
	src/session_manager.h
	src/shortcut_edit.h
	src/smart_quotes.h
	src/sound.h
	src/stack.h
	src/stats.h
	src/symbols_dialog.h
	src/symbols_model.h
	src/text_codec.h
	src/theme.h
	src/theme_dialog.h
	src/theme_manager.h
	src/theme_renderer.h
	src/timer.h
	src/timer_display.h
	src/timer_manager.h
	src/utils.h
	src/window.h
	src/fileformats/docx_reader.h
	src/fileformats/docx_writer.h
	src/fileformats/format_manager.h
	src/fileformats/html_writer.h
	src/fileformats/odt_reader.h
	src/fileformats/odt_writer.h
	src/fileformats/rtf_reader.h
	src/fileformats/rtf_tokenizer.h
	src/fileformats/rtf_writer.h
	src/fileformats/txt_reader.h
	src/spelling/abstract_dictionary.h
	src/spelling/abstract_dictionary_provider.h
	src/spelling/dictionary_dialog.h
	src/spelling/dictionary_manager.h
	src/spelling/dictionary_ref.h
	src/spelling/highlighter.h
	src/spelling/spell_checker.h
	src/3rdparty/qtzip/qtzipreader.h
	src/3rdparty/qtzip/qtzipwriter.h
	# Sources
	src/action_manager.cpp
	src/alert.cpp
	src/alert_layer.cpp
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
	src/3rdparty/qtzip/qtzip.cpp
	# Resources
	resources/images/images.qrc
	${translations_QM}
)

target_include_directories(focuswriter PRIVATE
	src
	src/fileformats
	src/spelling
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

# Create translations
file(GLOB translations_SRCS translations/*.ts)
qt_add_translations(focuswriter
	TS_FILES ${translations_SRCS}
	QM_FILES_OUTPUT_VARIABLE translations_QM
	LUPDATE_OPTIONS -no-obsolete -locations none
)

# Add spellchecking support
if(APPLE)
	target_sources(focuswriter PRIVATE
		src/spelling/dictionary_provider_nsspellchecker.h
		src/spelling/dictionary_provider_nsspellchecker.mm
	)

	find_library(APPKIT AppKit)
	target_link_libraries(focuswriter PRIVATE ${APPKIT})
elseif(WIN32)
	target_compile_definitions(focuswriter PRIVATE HUNSPELL_STATIC)

	target_include_directories(focuswriter PRIVATE src/3rdparty/hunspell)

	target_sources(focuswriter PRIVATE
		src/spelling/dictionary_provider_hunspell.h
		src/spelling/dictionary_provider_hunspell.cpp
		src/spelling/dictionary_provider_voikko.h
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

	if (Qt6_VERSION VERSION_GREATER_EQUAL 6.5)
		target_compile_definitions(focuswriter PRIVATE RTFCLIPBOARD)

		target_sources(focuswriter PRIVATE
			src/fileformats/clipboard_windows.h
			src/fileformats/clipboard_windows.cpp
		)
	endif()
else()
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(Hunspell REQUIRED IMPORTED_TARGET hunspell)

	target_sources(focuswriter PRIVATE
		src/spelling/dictionary_provider_hunspell.h
		src/spelling/dictionary_provider_hunspell.cpp
		src/spelling/dictionary_provider_voikko.h
		src/spelling/dictionary_provider_voikko.cpp
	)

	target_link_libraries(focuswriter PRIVATE PkgConfig::Hunspell)
endif()

# Make single instance
find_package(KDSingleApplication-qt6)
if(TARGET KDAB::kdsingleapplication)
	target_link_libraries(focuswriter PRIVATE KDAB::kdsingleapplication)
else()
	message(STATUS "Using 3rdparty KDSingleApplication")

	target_compile_definitions(focuswriter PRIVATE KDSINGLEAPPLICATION_STATIC_BUILD)

	target_include_directories(focuswriter PRIVATE src/3rdparty/kdsingleapplication)

	target_sources(focuswriter PRIVATE
		src/3rdparty/kdsingleapplication/kdsingleapplication.cpp
		src/3rdparty/kdsingleapplication/kdsingleapplication_localsocket.cpp
	)
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
	bundle_data(focuswriter ${CMAKE_SOURCE_DIR}/resources/symbols/symbols1700.dat Resources)
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
	install(FILES resources/symbols/symbols1700.dat DESTINATION ${CMAKE_INSTALL_DATADIR}/focuswriter)
	install(FILES resources/unix/focuswriter.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1 COMPONENT doc)

	include(cmake/TranslateMetainfo.cmake)
	process_and_install_metainfo(PO_DIR ${CMAKE_SOURCE_DIR}/resources/unix/po)
endif()

set_property(SOURCE src/main.cpp APPEND PROPERTY COMPILE_DEFINITIONS FOCUSWRITER_DATADIR="${datadir}")
