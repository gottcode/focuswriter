# SPDX-FileCopyrightText: 2021 Graeme Gott <graeme@gottcode.org>
#
# SPDX-License-Identifier: GPL-3.0-or-later

qt6_add_executable(generate main.cpp)

target_link_libraries(generate PRIVATE
	Qt6::Core
	Qt6::Network
)

add_custom_target(symbols
	COMMAND generate
	DEPENDS main.cpp
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
