cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)

project(Core)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/core/*.cpp src/cryptography/*.cpp src/util/*.cpp)
add_library(${PROJECT_NAME} STATIC ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src)

find_package(OpenSSL REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES})


project(Wallet)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/wallet/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src)
target_link_libraries(${PROJECT_NAME} PUBLIC Core)