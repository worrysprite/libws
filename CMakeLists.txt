CMAKE_MINIMUM_REQUIRED(VERSION 3.11)

# 必须在 project() 之前设置，vcpkg toolchain 才能生效
if (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if(DEFINED ENV{VCPKG_ROOT})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
	elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/extern/vcpkg/scripts/buildsystems/vcpkg.cmake")
		set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/extern/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
	endif()
endif()

PROJECT(libws)

SET(CMAKE_CXX_STANDARD_REQUIRED 20)
SET(CMAKE_CXX_STANDARD 20)

message(STATUS "Vcpkg toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

find_package(spdlog CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(unofficial-libmysql REQUIRED)
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

INCLUDE_DIRECTORIES("${PROJECT_SOURCE_DIR}/include")
if (MSVC)
SET(BUILD_SHARED_LIBS false)
add_compile_options("/std:c++20" "/utf-8" "/MP" "$<$<CONFIG:RELEASE>:/O2>")

elseif (CMAKE_COMPILER_IS_GNUCXX)
SET(BUILD_SHARED_LIBS false)
SET(CMAKE_INSTALL_PREFIX "/usr/local/libws")
add_compile_options("-std=c++20" "-Wall" "$<$<CONFIG:DEBUG>:-O0>" "$<$<CONFIG:DEBUG>:-g>" "$<$<CONFIG:DEBUG>:-rdynamic>" "$<$<CONFIG:RELEASE>:-O2>" "-fconcepts")

endif (MSVC)

#设置输出目录 库
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)

ADD_SUBDIRECTORY(wsCore)
ADD_SUBDIRECTORY(wsDatabase)
ADD_SUBDIRECTORY(wsNetwork)
ADD_SUBDIRECTORY(test)

#设置安装目录
INSTALL(TARGETS wsCore wsDatabase wsNetwork RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
INSTALL(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h")
