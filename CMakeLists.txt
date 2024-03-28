cmake_minimum_required(VERSION 3.0)
set(CMAKE_CXX_STANDARD 23)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(SOLUTION_NAME "Blockchain")

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
    set(USE_MSVC_RUNTIME_LIBRARY_DLL 1)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
    add_link_options(-no-pie)
endif()
set(CMAKE_FOLDER Libs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})


function(assign_source_group)
    foreach(source IN LISTS ${ARGN})
        file(RELATIVE_PATH source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${source}")
        get_filename_component(source_path "${source_rel}" PATH)
        string(REPLACE "/" "\\" source_path_msvc "${source_path}")
        source_group("${source_path_msvc}" FILES "${source}")
    endforeach()
endfunction(assign_source_group)

file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS src/*.cpp src/*.h)
assign_source_group(ALL_SOURCES)


### Core #########
project(core)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/core/*.cpp src/core/*.h)
add_library(${PROJECT_NAME} STATIC ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src/core)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${SOLUTION_NAME})

set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES} OpenSSL::Crypto)


### Test #########
project(test)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/test/*.cpp src/test/*.h)
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src/test)
target_link_libraries(${PROJECT_NAME} core)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${SOLUTION_NAME})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})


project(${SOLUTION_NAME})
