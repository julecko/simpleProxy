cmake_minimum_required(VERSION 3.10)
project(simpleProxy C)

include_directories(${PROJECT_SOURCE_DIR}/include)

# Core library
add_library(simpleProxyCore STATIC
    src/core/proxy.c
    src/core/util.c
    src/core/http.c
    src/core/https.c
    src/core/auth.c
    src/core/collections/hashset.c
)

find_package(Threads REQUIRED)
target_link_libraries(simpleProxyCore PUBLIC Threads::Threads)

# Executable 1: simpleProxy
add_executable(simpleProxy src/main/main.c)
target_link_libraries(simpleProxy PRIVATE simpleProxyCore)

# Executable 2: simpleProxyConfig
add_executable(simpleProxyConfig src/proxy_config/proxy_config.c)
target_link_libraries(simpleProxyConfig PRIVATE simpleProxyCore)
