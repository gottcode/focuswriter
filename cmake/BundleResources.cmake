# SPDX-FileCopyrightText: 2022 Graeme Gott <graeme@gottcode.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

# Add files to a macOS bundle.
function(bundle_data target source destination)
	if(IS_DIRECTORY ${source})
		# Recursively find files under source
		file(GLOB_RECURSE files RELATIVE ${source} ${source}/*)
		set(parent ${source})
	else()
		# Handle single file
		get_filename_component(files ${source} NAME)
		get_filename_component(parent ${source} DIRECTORY)
	endif()

	# Set each file to be located under destination
	foreach(resource ${files})
		get_filename_component(path ${resource} DIRECTORY)
		set_property(
			SOURCE ${parent}/${resource}
			PROPERTY
			MACOSX_PACKAGE_LOCATION ${destination}/${path}
		)
	endforeach()

	# Make target depend on resources
	list(TRANSFORM files PREPEND "${parent}/")
	target_sources(${target} PRIVATE ${files})
endfunction()

# Add translations to a macOS bundle.
function(bundle_translations target translations)
	foreach(file ${translations})
		# Set each translation to be located under Resources
		set_property(
			SOURCE ${file}
			PROPERTY
			MACOSX_PACKAGE_LOCATION Resources/translations
		)

		# Inform macOS about translation for native dialogs
		get_filename_component(resource ${file} NAME)
		string(REGEX REPLACE "[^_]*_([^\\.]*)\\..*" "\\1.lproj" lang ${resource})
		add_custom_command(
			TARGET ${target}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND}
			ARGS -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:${target}>/Resources/${lang}
		)
	endforeach()
endfunction()
