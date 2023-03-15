project(wsDatabase)

find_package(libmysql REQUIRED)

aux_source_directory(${PROJECT_SOURCE_DIR}/src wsdatabase_src)
add_library(${PROJECT_NAME} ${wsdatabase_src})

target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog spdlog::spdlog_header_only)