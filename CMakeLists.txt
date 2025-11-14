cmake_minimum_required(VERSION 3.10)
project(sgos)
set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig REQUIRED)

pkg_check_modules(GTKMM REQUIRED gtkmm-4.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB SOURCES "src/*.cc")

add_executable(main ${SOURCES})

target_include_directories(main PRIVATE
    ${GTKMM_INCLUDE_DIRS}
    ../include  
)


target_link_libraries(main PRIVATE sqlite3 ${GTKMM_LIBRARIES})
target_compile_options(main PRIVATE ${GTKMM_CFLAGS_OTHER})
