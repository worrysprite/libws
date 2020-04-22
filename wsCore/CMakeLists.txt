CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(wsCore)

if (MSVC)
add_compile_options("/std:c++17" "/utf-8" "/MP" "$<$<CONFIG:RELEASE>:/O2>")
else (CMAKE_COMPILER_IS_GNUCXX)
add_compile_options("-std=c++17" "-Wall" "$<$<CONFIG:DEBUG>:-g>" "$<$<CONFIG:RELEASE>:-O2>")
endif (MSVC)

AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/src wscore_src)
ADD_LIBRARY(${PROJECT_NAME} STATIC ${wscore_src})
