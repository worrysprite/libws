project(wsNetwork)

find_package(CURL CONFIG REQUIRED)

aux_source_directory(${PROJECT_SOURCE_DIR}/src wsNetwork_src)
add_library(${PROJECT_NAME} ${wsNetwork_src})

target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
#target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog spdlog::spdlog_header_only)