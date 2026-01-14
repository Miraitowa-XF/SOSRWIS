# 收集本目录下所有的 .cpp 和 .c 文件
file(GLOB_RECURSE SOURCES "*.cpp" "*.c")

# 创建 CoreLib 静态库
add_library(CoreLib ${SOURCES} )

# 【关键点】
# CoreLib 里的代码可能会用到 glad.h 或 glfw3.h
# 所以我们需要让 CoreLib 也能找到这些头文件
# 链接 glad 库会自动把 glad/include 路径带过来
target_link_libraries(CoreLib 
    PUBLIC 
    glad 
    imgui # 如果 CoreLib 封装了 UI 代码
)



# 手动添加 GLFW 的 include 路径 (如果 src 里的代码用到了 GLFW)
# 显式包含 Header-Only 库的路径
target_include_directories(CoreLib PUBLIC 
    "${CMAKE_SOURCE_DIR}/third_party/glm"           # 让 CoreLib 能用 GLM
    "${CMAKE_SOURCE_DIR}/third_party/stb_image/include" # 让 CoreLib 能用 stb_image
    "${CMAKE_SOURCE_DIR}/third_party/assimp/include"    # 让 CoreLib 能用 Assimp
    "${CMAKE_SOURCE_DIR}/third_party/glfw/include"
)

# 暴露 CoreLib 自己的路径，这样 main.cpp 引用 src 里的头文件时，
# 可以直接 #include "example.h" 而不用写 #include "src/example.h"
target_include_directories(CoreLib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})