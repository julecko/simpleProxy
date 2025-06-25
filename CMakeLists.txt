cmake_minimum_required(VERSION 3.10)
project(simpleProxy C)

include_directories(${PROJECT_SOURCE_DIR}/include)

# Detect MySQL client flags via mysql_config
execute_process(COMMAND mysql_config --cflags
    OUTPUT_VARIABLE MYSQL_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND mysql_config --libs
    OUTPUT_VARIABLE MYSQL_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE)

# Core library
add_library(simpleProxyCore STATIC
    src/core/proxy.c
    src/core/util.c
    src/core/http.c
    src/core/https.c
    src/core/auth.c
    src/core/collections/hashset.c
)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(simpleProxyCore PUBLIC Threads::Threads)

# Apply MySQL flags to the core (if used inside the core)
target_compile_options(simpleProxyCore PRIVATE ${MYSQL_CFLAGS})
target_link_libraries(simpleProxyCore PRIVATE ${MYSQL_LIBS})

# Expose posix
add_compile_definitions(_POSIX_C_SOURCE=200809L)

# Executable 1: simpleProxy
add_executable(simpleProxy src/main/main.c)
target_link_libraries(simpleProxy PRIVATE simpleProxyCore)

# Executable 2: simpleProxyConfig
add_executable(simpleProxyConfig src/proxy_config/proxy_config.c)
target_link_libraries(simpleProxyConfig PRIVATE simpleProxyCore)
