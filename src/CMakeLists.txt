file(GLOB_RECURSE SOURCE_FILES "*.cpp" "*.hpp")

add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES})