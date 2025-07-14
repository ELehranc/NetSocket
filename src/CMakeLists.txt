aux_source_directory(. SRC_LIST)
aux_source_directory(./net NET_LIST)
aux_source_directory(./base BASE_LIST)

# 指定生成可执行文件
add_executable(NetSocket ${SRC_LIST} ${NET_LIST} ${BASE_LIST})

target_link_libraries(NetSocket pthread)