PROJECT(wsCore)

AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/src wscore_src)
ADD_LIBRARY(${PROJECT_NAME} ${wscore_src})

TARGET_LINK_LIBRARIES(${PROJECT_NAME}
	PUBLIC
		spdlog::spdlog
		OpenSSL::SSL
		OpenSSL::Crypto
		Threads::Threads
)
