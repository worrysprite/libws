cmake_minimum_required(VERSION 3.16)
project(libws)

set(CMAKE_CXX_STANDARD_REQUIRED 20)
set(CMAKE_CXX_STANDARD 20)

find_package(spdlog CONFIG REQUIRED)

include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include")

set(BUILD_SHARED_LIBS false)
set(LIBRARY_OUTPUT_PATH "${PROJECT_SOURCE_DIR}/build/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/bin")

add_compile_definitions("$<$<CONFIG:Debug>:_DEBUG>" "$<$<CONFIG:Release>:NDEBUG>")

if (MSVC)
add_compile_definitions("_CONSOLE" "WIN32_LEAN_AND_MEAN" "NOMINMAX" "_CRT_SECURE_NO_WARNINGS" "_WIN32_WINNT=0x0601")
add_compile_options("/std:c++20" "/utf-8" "/MP" "$<$<CONFIG:Release>:/O2>")
elseif (CMAKE_COMPILER_IS_GNUCXX)
set(CMAKE_INSTALL_PREFIX "/usr/local/libws")
add_compile_options("-std=c++20" "-Wall" "$<$<CONFIG:Debug>:-O0>" "$<$<CONFIG:Debug>:-rdynamic>" "$<$<CONFIG:Release>:-O2>")
endif (MSVC)

add_subdirectory(wsCore)
add_subdirectory(wsDatabase)
add_subdirectory(wsNetwork)
add_subdirectory(test)

#设置安装目录
install(TARGETS wsCore wsDatabase wsNetwork RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION include FILES_MATCHING PATTERN "*.h")
