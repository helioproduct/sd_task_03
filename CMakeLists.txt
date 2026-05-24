cmake_minimum_required(VERSION 3.12)
project(mail-lab CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(USERVER_FEATURE_POSTGRESQL ON CACHE BOOL "" FORCE)
set(USERVER_FEATURE_PATCH_LIBPQ OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_TESTSUITE OFF CACHE BOOL "" FORCE)
set(USERVER_FEATURE_FORMAT OFF CACHE BOOL "" FORCE)

find_package(userver REQUIRED COMPONENTS core postgresql)
find_package(PostgreSQL REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/api/http_handlers.cpp
    src/domain/entities.cpp
    src/domain/mail_repository.cpp
    src/domain/jwt_tools.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${PostgreSQL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    userver::core
    userver::postgresql
    ${PostgreSQL_LIBRARIES}
)
