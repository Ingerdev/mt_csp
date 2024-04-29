# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
#mute modern cmake compatibility issue
cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0079 NEW)

add_library(csp Accountant.cpp
        include/csp/utils.h
        include/csp/WorkerPool.h
        include/csp/FileReader.h
        Worker.cpp)

target_include_directories(csp PUBLIC include)

set(Boost_NO_WARN_NEW_VERSIONS ON)
set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

#20 - for std::format
target_compile_features (csp PUBLIC cxx_std_20)

find_package(
  Boost REQUIRED
  COMPONENTS fiber
             filesystem
             program_options
             thread)

target_link_libraries(
        csp
  PUBLIC Boost::fiber
          Boost::filesystem
          Boost::program_options
          Boost::thread)

