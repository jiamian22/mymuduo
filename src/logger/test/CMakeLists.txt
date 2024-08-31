set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/src/logger/test)

add_executable(AsyncLoggingTest AsyncLoggingTest.cpp)

target_link_libraries(AsyncLoggingTest mymuduo)