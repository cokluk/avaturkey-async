cmake_minimum_required(VERSION 3.16)
project(avaturkey
    VERSION 1.0.0
    DESCRIPTION "Avataria-style TCP game server with binary protocol and Redis persistence"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -O2)
endif()

include(FetchContent)
FetchContent_Declare(
    hiredis
    GIT_REPOSITORY https://github.com/redis/hiredis.git
    GIT_TAG v1.3.0
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ENABLE_SSL OFF CACHE BOOL "" FORCE)
set(DISABLE_TESTS ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(hiredis)

set(SOURCES
    src/main.c
    src/buf.c
    src/value.c
    src/protocol.c
    src/client.c
    src/server.c
    src/redis_ctx.c
    src/module.c
    src/common.c
    src/const.c
    modules/mod_client_error.c
    modules/mod_confirm.c
    modules/mod_stub.c
)

add_executable(avaturkey ${SOURCES})
target_include_directories(avaturkey PRIVATE include modules)

target_link_libraries(avaturkey PRIVATE hiredis)
target_include_directories(avaturkey PRIVATE ${hiredis_SOURCE_DIR})

if(WIN32)
    target_link_libraries(avaturkey PRIVATE ws2_32)
endif()
