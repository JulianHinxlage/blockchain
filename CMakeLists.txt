cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)

project(Test)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src)

find_package(OpenSSL REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OPENSSL_LIBRARIES})