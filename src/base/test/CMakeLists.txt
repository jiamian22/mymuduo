set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/base/test)

add_executable(ThreadPoolTest ThreadPoolTest.cpp)

target_link_libraries(ThreadPoolTest mymuduo)