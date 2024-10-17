cmake_minimum_required(VERSION 3.00)
set(CMAKE_CXX_STANDARD 20)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(SOLUTION_NAME "Blockchain")

if(WIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
    set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
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
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PUBLIC ws2_32)
else()
    target_link_libraries(${PROJECT_NAME} PUBLIC pthread)
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${SOLUTION_NAME})

#set(OPENSSL_USE_STATIC_LIBS TRUE)
find_package(OpenSSL REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES} OpenSSL::Crypto)



### Core cAPI #########
project(core_capi)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/capi/*.cpp src/capi/*.h)
add_library(${PROJECT_NAME} SHARED ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src/capi)
target_link_libraries(${PROJECT_NAME} core)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${SOLUTION_NAME})


### Wallet #########
project(wallet)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/wallet/*.cpp src/wallet/*.h)
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src/wallet)
target_link_libraries(${PROJECT_NAME} core)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${SOLUTION_NAME})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

### Validator #########
project(validator)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/validator/*.cpp src/validator/*.h)
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src/validator)
target_link_libraries(${PROJECT_NAME} core)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${SOLUTION_NAME})

### UI Wallet #########
if(WIN32)
project(ui_wallet)
set(CMAKE_PREFIX_PATH "C:/qt")
find_package(Qt6 COMPONENTS Core Widgets)
if(Qt6_FOUND)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/ui_wallet/*.cpp src/ui_wallet/*.h)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src/ui_wallet)
target_link_libraries(${PROJECT_NAME} core)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${SOLUTION_NAME})
target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets)
else()
add_custom_target(ui_wallet
    COMMAND ${CMAKE_COMMAND} -E echo "Error: QT Library not found. This target cannot be built."
    COMMAND ${CMAKE_COMMAND} -E false
)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${SOLUTION_NAME})
endif()
endif()

### Test #########
project(test)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/test/*.cpp src/test/*.h)
add_executable(${PROJECT_NAME} ${SOURCES})
include_directories(${PROJECT_NAME} PUBLIC src/test src)
target_link_libraries(${PROJECT_NAME} core)
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${SOLUTION_NAME})

project(${SOLUTION_NAME})
