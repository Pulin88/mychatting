# 定义SRC_LIST变量，包含所有源文件
aux_source_directory(. SRC_LIST)

# 指定生成可执行文件
add_executable(ChatClient ${SRC_LIST})
# 指定链接的依赖库文件
target_link_libraries(ChatClient pthread)