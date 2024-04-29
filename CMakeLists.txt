# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)
cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0079 NEW)

project (CSP_example VERSION 0.1 LANGUAGES CXX)
add_executable (${PROJECT_NAME}  ${PROJECT_SOURCE_DIR}/src/main.cpp)

enable_testing()


add_subdirectory(libs)
add_subdirectory(tests)

# Add source to this project's executable.
target_sources(${PROJECT_NAME}
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(${PROJECT_NAME}
        PRIVATE
        csp)
