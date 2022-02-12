# 注意：跑到build目录下执行shell命令 cmake .. 中间结果保留在build，其他的文件在 .. 中寻找
# 注意：若是
cmake_minimum_required(VERSION 2.8)
project(muduo_test)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGES} -g)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# include_directories()
# link_directories()
# add_library(sum SHARE/STATIC sum.cpp)
add_executable(muduo_server muduo_server.cpp)
target_link_libraries(muduo_server muduo_net muduo_base pthread)
