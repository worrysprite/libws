project(wsCore)

#find_package(OpenSSL REQUIRED)

aux_source_directory(${PROJECT_SOURCE_DIR}/src core_src)
add_library(${PROJECT_NAME} ${core_src})

target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog spdlog::spdlog_header_only)