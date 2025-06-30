cmake_minimum_required(VERSION 3.10)
project(simpleProxy C)

# Set default to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

include_directories(${PROJECT_SOURCE_DIR}/include)

# Detect MySQL client flags via mysql_config
execute_process(COMMAND mysql_config --cflags
    OUTPUT_VARIABLE MYSQL_CFLAGS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(MYSQL_CFLAGS)

execute_process(COMMAND mysql_config --libs
    OUTPUT_VARIABLE MYSQL_LIBS
    OUTPUT_STRIP_TRAILING_WHITESPACE)
separate_arguments(MYSQL_LIBS)

# Core library (SHARED)
add_library(simpleProxyCore SHARED
    src/core/proxy.c
    src/core/util.c
    src/core/http.c
    src/core/https.c
    src/core/auth.c
    src/core/collections/hashset.c
    src/core/db/db.c
    src/core/db/migration.c
    src/core/db/user.c
    src/core/db/util.c
)

# Threads
find_package(Threads REQUIRED)
target_link_libraries(simpleProxyCore PUBLIC Threads::Threads)

# Apply MySQL flags to the core
target_compile_options(simpleProxyCore PRIVATE ${MYSQL_CFLAGS})
target_link_libraries(simpleProxyCore PRIVATE ${MYSQL_LIBS})

# Enable link-time optimization (interprocedural)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Debug/Release compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(simpleProxyCore PRIVATE DEBUG_MODE=1)
    target_compile_options(simpleProxyCore PRIVATE -g -O0)
else()
    target_compile_definitions(simpleProxyCore PRIVATE NDEBUG)
    target_compile_options(simpleProxyCore PRIVATE -O3)
endif()

# Executable 1: simpleProxy
add_executable(simpleProxy src/main/main.c)
target_link_libraries(simpleProxy PRIVATE simpleProxyCore)
set_target_properties(simpleProxy PROPERTIES BUILD_RPATH "$ORIGIN")

# Executable 2: simpleProxyConfig
add_executable(simpleProxyConfig src/proxy_config/proxy_config.c)
target_link_libraries(simpleProxyConfig PRIVATE simpleProxyCore ${MYSQL_LIBS})
set_target_properties(simpleProxyConfig PROPERTIES BUILD_RPATH "$ORIGIN")


if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CPACK_PACKAGE_NAME "simpleproxy")
    set(CPACK_PACKAGE_VERSION "1.0.0")
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Julecko <julecko@dilino.sk>")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Lightweight HTTP/HTTPS proxy with authentication")

    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.28), libmysqlclient21, libssl3, libzstd1, libstdc++6, libgcc1")

    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
        "${CMAKE_SOURCE_DIR}/packaging/postinst"
    )

    include(CPack)

    install(TARGETS simpleProxy DESTINATION bin)
    install(TARGETS simpleProxyConfig DESTINATION bin)
    install(TARGETS simpleProxyCore
        LIBRARY DESTINATION lib
    )

    install(FILES ${CMAKE_SOURCE_DIR}/packaging/config.ini DESTINATION etc/simpleproxy)

    install(FILES ${CMAKE_SOURCE_DIR}/packaging/simpleproxy.service DESTINATION lib/systemd/system)

endif()