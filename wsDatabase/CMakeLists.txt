PROJECT(wsDatabase)

if (MSVC)
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/3rd/mysql/include")
elseif (CMAKE_COMPILER_IS_GNUCXX)
INCLUDE_DIRECTORIES("/usr/include/mysql")
endif (MSVC)

AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/src wsdatabase_src)
ADD_LIBRARY(${PROJECT_NAME} ${wsdatabase_src})
