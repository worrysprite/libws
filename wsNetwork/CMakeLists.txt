PROJECT(wsNetwork)

AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/src wsNetwork_src)
ADD_LIBRARY(${PROJECT_NAME} ${wsNetwork_src})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
	PUBLIC
		wsCore
		spdlog::spdlog
		CURL::libcurl
)
