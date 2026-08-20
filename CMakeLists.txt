CMAKE_MINIMUM_REQUIRED(VERSION 3.11)

# 顶层独立编译时才自行设置 toolchain；作为 submodule 时沿用父项目
set(LIBWS_IS_TOP_LEVEL OFF)
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	set(LIBWS_IS_TOP_LEVEL ON)
endif ()

if (LIBWS_IS_TOP_LEVEL AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if (DEFINED ENV{VCPKG_ROOT})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
	elseif (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/vcpkg/scripts/buildsystems/vcpkg.cmake")
		set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/extern/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
	endif ()
endif ()

PROJECT(libws)

SET(CMAKE_CXX_STANDARD_REQUIRED 20)
SET(CMAKE_CXX_STANDARD 20)

message(STATUS "Vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Windows：vcpkg libmysql；Linux：系统 libmysqlclient / libmariadb
if (MSVC)
	find_package(unofficial-libmysql REQUIRED)
	set(LIBWS_MYSQL_TARGET unofficial::libmysql::libmysql)
else ()
	find_path(LIBWS_MYSQL_INCLUDE_DIR mysql.h
		PATH_SUFFIXES mysql mariadb)
	find_library(LIBWS_MYSQL_LIBRARY NAMES mysqlclient mariadb libmariadb)
	if (NOT LIBWS_MYSQL_INCLUDE_DIR OR NOT LIBWS_MYSQL_LIBRARY)
		message(FATAL_ERROR "未找到系统 MySQL 客户端库，请安装 libmysqlclient-dev / libmariadb-dev")
	endif ()
	add_library(libws_system_mysql INTERFACE)
	target_include_directories(libws_system_mysql INTERFACE "${LIBWS_MYSQL_INCLUDE_DIR}")
	target_link_libraries(libws_system_mysql INTERFACE "${LIBWS_MYSQL_LIBRARY}")
	set(LIBWS_MYSQL_TARGET libws_system_mysql)
	message(STATUS "libws: using system MySQL (${LIBWS_MYSQL_LIBRARY})")
endif ()

if (MSVC)
	SET(BUILD_SHARED_LIBS false)
	add_compile_options("/std:c++20" "/utf-8" "/MP" "$<$<CONFIG:RELEASE>:/O2>")
elseif (CMAKE_COMPILER_IS_GNUCXX)
	SET(BUILD_SHARED_LIBS false)
	if (LIBWS_IS_TOP_LEVEL)
		SET(CMAKE_INSTALL_PREFIX "/usr/local/libws")
	endif ()
	add_compile_options("-std=c++20" "-Wall" "$<$<CONFIG:DEBUG>:-O0>" "$<$<CONFIG:DEBUG>:-g>" "$<$<CONFIG:DEBUG>:-rdynamic>" "$<$<CONFIG:RELEASE>:-O2>" "-fconcepts")
endif (MSVC)

if (LIBWS_IS_TOP_LEVEL)
	SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)
	SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)
endif ()

ADD_SUBDIRECTORY(wsCore)
ADD_SUBDIRECTORY(wsDatabase)
ADD_SUBDIRECTORY(wsNetwork)

foreach (_libws_tgt wsCore wsDatabase wsNetwork)
	target_include_directories(${_libws_tgt} PUBLIC "${PROJECT_SOURCE_DIR}/include")
endforeach ()

option(LIBWS_BUILD_TESTS "Build libws tests" ${LIBWS_IS_TOP_LEVEL})
if (LIBWS_BUILD_TESTS)
	ADD_SUBDIRECTORY(test)
endif ()

INSTALL(TARGETS wsCore wsDatabase wsNetwork RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h")
