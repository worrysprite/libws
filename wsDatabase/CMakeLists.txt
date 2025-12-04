PROJECT(wsDatabase)

if (MSVC)
find_package(libmysql CONFIG REQUIRED)
elseif (CMAKE_COMPILER_IS_GNUCXX)
INCLUDE_DIRECTORIES("/usr/include/mysql")
endif (MSVC)

AUX_SOURCE_DIRECTORY(${PROJECT_SOURCE_DIR}/src wsdatabase_src)
ADD_LIBRARY(${PROJECT_NAME} ${wsdatabase_src})
