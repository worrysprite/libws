PROJECT(test)

find_package(libmysql REQUIRED)

AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/src test_src)

ADD_EXECUTABLE(${PROJECT_NAME} ${test_src})
target_link_libraries(${PROJECT_NAME} PRIVATE wsCore)
target_link_libraries(${PROJECT_NAME} PRIVATE wsDatabase)
target_link_libraries(${PROJECT_NAME} PRIVATE wsNetwork)
target_link_libraries(${PROJECT_NAME} PRIVATE ${MYSQL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} PRIVATE spdlog::spdlog spdlog::spdlog_header_only)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto)

if (CMAKE_COMPILER_IS_GNUCXX)
target_link_libraries(${PROJECT_NAME} pthread)
target_link_libraries(${PROJECT_NAME} dl)
endif (CMAKE_COMPILER_IS_GNUCXX)
