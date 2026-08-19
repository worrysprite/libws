PROJECT(wsDatabase)

AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/src wsdatabase_src)
ADD_LIBRARY(${PROJECT_NAME} ${wsdatabase_src})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
	PUBLIC
		wsCore
		unofficial::libmysql::libmysql
		spdlog::spdlog
)
