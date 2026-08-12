cmake_minimum_required(VERSION 3.10)
project(SFTP_Project C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Бинарник
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})

# Опции сборки
option(BUILD_CLIENT "Build SFTP client" ON)
option(BUILD_SERVER "Build SFTP server" ON)

set(COMMON_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/common/include)

# Клиент
if(BUILD_CLIENT)
    add_executable(sftp_client
        client/src/client.c
        client/src/client_auth.c
        client/src/client_cli.c
        client/src/client_file.c
        client/src/client_inet.c
    )
    
    target_include_directories(sftp_client PRIVATE
        ${CMAKE_SOURCE_DIR}/client/include
        ${COMMON_INCLUDE_DIR}
    )
    
    target_compile_options(sftp_client PRIVATE -Wall -Wextra)
endif()

# Сервер
if(BUILD_SERVER)
    add_executable(sftp_server
        server/src/server.c
        server/src/auth.c
        server/src/client_handler.c
        server/src/config.c
        server/src/generators.c
        server/src/log.c
        server/src/network.c
    )
    
    target_include_directories(sftp_server PRIVATE
        ${CMAKE_SOURCE_DIR}/server/include
        ${COMMON_INCLUDE_DIR}
    )
    
    target_compile_options(sftp_server PRIVATE -Wall -Wextra)
endif()

# Отладка
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(BUILD_CLIENT)
        target_compile_options(sftp_client PRIVATE -g -O0)
    endif()
    if(BUILD_SERVER)
        target_compile_options(sftp_server PRIVATE -g -O0)
    endif()
endif()

# Инфо
message(STATUS "Build client: ${BUILD_CLIENT}")
message(STATUS "Build server: ${BUILD_SERVER}")
message(STATUS "Binary directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")