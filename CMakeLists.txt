cmake_minimum_required(VERSION 3.20)
project(Bind VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Static CRT linking — no external DLLs needed
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

include_directories(${CMAKE_SOURCE_DIR}/include)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(WIN32_LEAN_AND_MEAN)
endif()

set(COMMON_SOURCES
    src/common/String.cpp
    src/common/Json.cpp
    src/common/ResultSet.cpp
    src/common/ArrayList.cpp
    src/common/LinkedList.cpp
)

set(STORAGE_SOURCES
    src/storage/BPlusTree.cpp
    src/storage/FileEngine.cpp
    src/storage/Table.cpp
    src/storage/Database.cpp
)

set(PARSER_SOURCES
    src/parser/SQLParser.cpp
)

set(EXECUTOR_SOURCES
    src/executor/Executor.cpp
)

set(NETWORK_SOURCES
    src/network/TcpSocket.cpp
)

set(SERVER_SOURCES
    src/server/Server.cpp
)

set(CLIENT_SOURCES
    src/client/Client.cpp
)

set(ALL_LIB_SOURCES
    ${COMMON_SOURCES}
    ${STORAGE_SOURCES}
    ${PARSER_SOURCES}
    ${EXECUTOR_SOURCES}
)

add_library(bind_lib STATIC ${ALL_LIB_SOURCES})
target_compile_features(bind_lib PUBLIC cxx_std_23)

add_executable(bind_server
    src/main_server.cpp
    ${SERVER_SOURCES}
    ${NETWORK_SOURCES}
)
target_link_libraries(bind_server bind_lib)
if(WIN32)
    target_link_libraries(bind_server ws2_32)
endif()

add_executable(bind_client
    src/main_client.cpp
    ${CLIENT_SOURCES}
    ${NETWORK_SOURCES}
)
target_link_libraries(bind_client bind_lib)
if(WIN32)
    target_link_libraries(bind_client ws2_32)
endif()
