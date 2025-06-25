cmake_minimum_required(VERSION 3.10)
project(simpleProxy C)

add_executable(simpleProxy
    src/main.c
    src/proxy.c
    src/util.c
    src/http.c
    src/https.c
    src/auth.c
    src/collections/hashset.c
)

target_include_directories(simpleProxy PUBLIC ${PROJECT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)
target_link_libraries(simpleProxy PRIVATE Threads::Threads)