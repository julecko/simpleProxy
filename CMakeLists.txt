cmake_minimum_required(VERSION 3.10)
project(simpleProxy C)

set(SIMPLEPROXY_VERSION "2.0.0")

# Set default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# Detect MySQL client flags via mysql_config
execute_process(COMMAND mysql_config --cflags
    OUTPUT_VARIABLE MYSQL_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(MYSQL_CFLAGS)

execute_process(COMMAND mysql_config --libs
    OUTPUT_VARIABLE MYSQL_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(MYSQL_LIBS)

# Enable link-time optimization (interprocedural)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Core library (SHARED)
add_library(simpleproxycore SHARED
    src/core/util.c
    src/core/config.c
    src/core/logger.c
    src/core/collections/hashmap.c
    src/core/db/db.c
    src/core/db/migration.c
    src/core/db/user.c
    src/core/db/util.c
)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(simpleproxycore PUBLIC Threads::Threads)

# Apply MySQL flags to the core
target_compile_options(simpleproxycore PRIVATE ${MYSQL_CFLAGS})
target_link_libraries(simpleproxycore PRIVATE ${MYSQL_LIBS})

# Include core headers globally for everything linking simpleproxycore
target_include_directories(simpleproxycore PUBLIC
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/core
)

# Debug/Release compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_MODE=1)
    add_compile_options(-g -O0)
else()
    add_compile_definitions(NDEBUG)
    add_compile_options(-O3)
endif()

# Executable 1: simpleProxy
add_executable(simpleproxy
    src/main/main.c
    src/main/proxy.c
    src/main/connection.c
    src/main/auth.c
    src/main/client.c
    src/main/util.c
    src/main/epoll_util.c
    src/main/loop_handlers.c
    src/main/timeout.c
)

target_include_directories(simpleproxy PRIVATE
    ${PROJECT_SOURCE_DIR}/include/main
)

target_compile_definitions(simpleproxycore PUBLIC VERSION=\"${SIMPLEPROXY_VERSION}\")

target_link_libraries(simpleproxy PRIVATE simpleproxycore ${MYSQL_LIBS})
set_target_properties(simpleproxy PROPERTIES BUILD_RPATH "$ORIGIN")

# Executable 2: simpleProxyConfig
add_executable(simpleproxy-config
    src/proxy_config/proxy_config.c
    src/proxy_config/commands.c
    src/proxy_config/setup.c
)

target_include_directories(simpleproxy-config PRIVATE
    ${PROJECT_SOURCE_DIR}/include/proxy_config
)

target_link_libraries(simpleproxy-config PRIVATE simpleproxycore ${MYSQL_LIBS})
set_target_properties(simpleproxy-config PROPERTIES BUILD_RPATH "$ORIGIN")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CPACK_PACKAGE_NAME "simpleproxy")
    set(CPACK_PACKAGE_VERSION "${SIMPLEPROXY_VERSION}")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Julecko <julecko@dilino.sk>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Lightweight HTTP/HTTPS proxy with authentication")

    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.28), libmariadb3, libssl3, libzstd1, libstdc++6, libgcc1")

    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_SOURCE_DIR}/packaging/conffiles"
        "${CMAKE_SOURCE_DIR}/packaging/postinst"
    )


    include(CPack)

    install(TARGETS simpleproxy DESTINATION bin)
    install(TARGETS simpleproxy-config DESTINATION bin)
    install(TARGETS simpleproxycore
        LIBRARY DESTINATION lib
    )

    install(FILES ${CMAKE_SOURCE_DIR}/packaging/simpleproxy.service DESTINATION lib/systemd/system)
    install(FILES ${CMAKE_SOURCE_DIR}/packaging/simpleproxy.conf DESTINATION /etc/simpleproxy)

endif()
