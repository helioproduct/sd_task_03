cmake_minimum_required(VERSION 3.12)
project(mail-lab CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(userver REQUIRED COMPONENTS core)
find_package(SQLite3 REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/api/http_handlers.cpp
    src/domain/entities.cpp
    src/domain/mail_repository.cpp
    src/domain/jwt_tools.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${SQLITE3_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    userver::core
    /usr/lib/x86_64-linux-gnu/libsqlite3.so
)
