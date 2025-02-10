# SPDX-FileCopyrightText: 2025 Graeme Gott <graeme@gottcode.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

function(process_and_install_metainfo name po_dir)
	find_package(Gettext 0.19.8 REQUIRED)

	string(TOLOWER "${name}" prefix)

	# Generate LINGUAS file
	file(GLOB po_files ${po_dir}/*.po)
	foreach(po_file ${po_files})
		get_filename_component(lang ${po_file} NAME_WE)
		list(APPEND linguas ${lang})
	endforeach()
	add_custom_command(
		OUTPUT ${po_dir}/LINGUAS
		COMMAND ${CMAKE_COMMAND} -E echo "${linguas}" > ${po_dir}/LINGUAS
		COMMAND_EXPAND_LISTS
		COMMENT "Generating LINGUAS"
	)

	# Generate desktop file
	set(desktop_file "${prefix}.desktop")
	add_custom_command(
		OUTPUT ${desktop_file}
		COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
			--desktop
			--template=${po_dir}/../${desktop_file}.in
			-d ${po_dir}
			-o ${desktop_file}
		DEPENDS ${po_files} ${po_dir}/LINGUAS
	)
	install(
		FILES ${CMAKE_CURRENT_BINARY_DIR}/${desktop_file}
		DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
	)
	list(APPEND metainfo_files ${desktop_file})

	# Generate AppData file
	set(appdata_file "${prefix}.appdata.xml")
	add_custom_command(
		OUTPUT ${appdata_file}
		COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
			--xml
			--template=${po_dir}/../${appdata_file}.in
			-d ${po_dir}
			-o ${appdata_file}
		DEPENDS ${po_files} ${po_dir}/LINGUAS
	)
	install(
		FILES ${CMAKE_CURRENT_BINARY_DIR}/${appdata_file}
		DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
	)
	list(APPEND metainfo_files ${appdata_file})

	# Generate mimetype file
	set(mimetype_file "${prefix}.xml")
	if(EXISTS "${po_dir}/../${mimetype_file}.in")
		add_custom_command(
			OUTPUT ${mimetype_file}
			COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
				--xml
				--template=${po_dir}/../${mimetype_file}.in
				-d ${po_dir}
				-o ${mimetype_file}
			DEPENDS ${po_files} ${po_dir}/LINGUAS
		)
		install(
			FILES ${CMAKE_CURRENT_BINARY_DIR}/${mimetype_file}
			DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages
		)
		list(APPEND metainfo_files ${mimetype_file})
	endif()

	# Generate description template
	find_program(XGETTEXT_EXECUTABLE xgettext)
	if(XGETTEXT_EXECUTABLE)
		add_custom_target(update_description_template
			COMMAND ${XGETTEXT_EXECUTABLE}
				--output=description.pot
				--from-code=UTF-8
				--package-name='${name}'
				--copyright-holder='Graeme Gott'
				../*.in
			WORKING_DIRECTORY ${po_dir}
			COMMENT "Generating description.pot"
		)
	endif()

	# Translate metainfo files
	add_custom_target(metainfo ALL DEPENDS ${metainfo_files})
endfunction()
