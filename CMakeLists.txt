cmake_minimum_required(VERSION 3.10)
project(simpleProxy C)

add_executable(simpleProxy
    src/main.c
    src/proxy.c
    src/util.c
)

target_include_directories(simpleProxy PUBLIC ${PROJECT_SOURCE_DIR}/include)
