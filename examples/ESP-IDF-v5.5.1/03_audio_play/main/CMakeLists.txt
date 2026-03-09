# 初始化源文件和包含目录
set(SOURCES
    "./main.c" 

)

set(INCLUDE_DIRS 
    "."
)

# 注册组件
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
)