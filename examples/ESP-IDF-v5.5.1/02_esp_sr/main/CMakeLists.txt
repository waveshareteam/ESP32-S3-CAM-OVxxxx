# 初始化源文件和包含目录
set(SOURCES
    "./main.c" 
    "./speech_det_driver/mic_speech.c"
)

set(INCLUDE_DIRS 
    "."
    "./speech_det_driver"
)



# 注册组件
idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS ${INCLUDE_DIRS}
)