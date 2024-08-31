# 加载子目录
set(
    echoServer.cpp
    # echoServerAsync.cpp
)

add_executable(echoServer echoServer.cpp)
# add_executable(echoServerAsync echoServerAsync.cpp)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/example)

target_link_libraries(echoServer mymuduo)
# target_link_libraries(echoServerAsync mymuduo)