cmake_minimum_required(VERSION 3.17)
project(dircmp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS -O3)
find_package(Boost COMPONENTS crc)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
add_executable(dircmp main.cpp file_h.h db_wrapper.cpp db_wrapper.h auxullary.h hashf.h fs_view.cpp fs_view.h fs_comparator.cpp fs_comparator.h)
find_package (SQLite3)
set(CMAKE_CXX_FLAGS -pthread)
if (SQLITE3_FOUND)
    include_directories(${SQLite3_INCLUDE_DIRS})
    target_link_libraries (dircmp ${SQLite3_LIBRARIES})
    message(${SQLite3_VERSION})
endif (SQLITE3_FOUND)



