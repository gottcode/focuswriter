# SPDX-FileCopyrightText: 2025 Graeme Gott <graeme@gottcode.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

function(process_and_install_metainfo)
	cmake_parse_arguments(PARSE_ARGV 0 arg "" "PO_DIR" "MIMETYPES")

	find_package(Gettext 0.19.8 REQUIRED)

	# Generate LINGUAS file
	file(GLOB po_files ${arg_PO_DIR}/*.po)
	foreach(po_file ${po_files})
		get_filename_component(lang ${po_file} NAME_WE)
		list(APPEND linguas ${lang})
	endforeach()
	add_custom_command(
		OUTPUT ${arg_PO_DIR}/LINGUAS
		COMMAND ${CMAKE_COMMAND} -E echo "${linguas}" > ${arg_PO_DIR}/LINGUAS
		COMMAND_EXPAND_LISTS
		COMMENT "Generating LINGUAS"
	)

	# Generate desktop file
	set(desktop_file "${PROJECT_NAME}.desktop")
	add_custom_command(
		OUTPUT ${desktop_file}
		COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
			--desktop
			--template=${arg_PO_DIR}/../${desktop_file}.in
			-d ${arg_PO_DIR}
			-o ${desktop_file}
		DEPENDS ${arg_PO_DIR}/../${desktop_file}.in ${po_files} ${arg_PO_DIR}/LINGUAS
	)
	install(
		FILES ${CMAKE_CURRENT_BINARY_DIR}/${desktop_file}
		DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
	)
	list(APPEND metainfo_files ${desktop_file})

	# Generate AppData file
	set(appdata_file "${PROJECT_NAME}.appdata.xml")
	add_custom_command(
		OUTPUT ${appdata_file}
		COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
			--xml
			--template=${arg_PO_DIR}/../${appdata_file}.in
			-d ${arg_PO_DIR}
			-o ${appdata_file}
		DEPENDS ${arg_PO_DIR}/../${appdata_file}.in ${po_files} ${arg_PO_DIR}/LINGUAS
	)
	install(
		FILES ${CMAKE_CURRENT_BINARY_DIR}/${appdata_file}
		DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
	)
	list(APPEND metainfo_files ${appdata_file})

	# Generate mimetype files
	foreach(mimetype_file ${arg_MIMETYPES})
		add_custom_command(
			OUTPUT ${mimetype_file}
			COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
				--xml
				--template=${arg_PO_DIR}/../${mimetype_file}.in
				-d ${arg_PO_DIR}
				-o ${mimetype_file}
			DEPENDS ${arg_PO_DIR}/../${mimetype_file}.in ${po_files} ${arg_PO_DIR}/LINGUAS
		)
		install(
			FILES ${CMAKE_CURRENT_BINARY_DIR}/${mimetype_file}
			DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages
		)
		list(APPEND metainfo_files ${mimetype_file})
	endforeach()

	# Generate description template
	find_program(XGETTEXT_EXECUTABLE xgettext)
	if(XGETTEXT_EXECUTABLE)
		add_custom_target(update_description_template
			COMMAND ${XGETTEXT_EXECUTABLE}
				--output=description.pot
				--from-code=UTF-8
				--package-name='${PROJECT_NAME}'
				--copyright-holder='Graeme Gott'
				../*.in
			WORKING_DIRECTORY ${arg_PO_DIR}
			COMMENT "Generating description.pot"
		)
	endif()

	# Translate metainfo files
	add_custom_target(metainfo ALL DEPENDS ${metainfo_files})
endfunction()
