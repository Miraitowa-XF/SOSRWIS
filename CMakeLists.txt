# 需求的最低cmake程序版本
cmake_minimum_required(VERSION 3.12)

# 本工程的名字
project(SOSRWIS)

# 本工程支持的C++版本
set(CMAKE_CXX_STANDARD 17)

# ==========================================
# 1. 配置第三方库 (Third Party Setup)
# ==========================================

# --- 配置 GLAD ---
# 将 glad.c 编译成一个静态库，名字叫 glad
add_library(glad STATIC "third_party/glad/src/glad.c")
# 告诉 glad 库，它的头文件在哪里
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include")

# --- 配置 GLFW ---
# GLFW 已经是编译好的 .lib 文件，我们只需要导入它
# 设置 GLFW 的头文件路径变量
set(GLFW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include")
# 设置 GLFW 的库文件路径变量
set(GLFW_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/lib")
# 找到具体的库文件 (注意文件名是否完全匹配)
find_library(GLFW_LIB NAMES glfw3 PATHS ${GLFW_LIB_DIR} NO_DEFAULT_PATH)

# --- 3. GLM (Header Only) ---
# GLM 不需要编译，只需要告诉编译器头文件在哪
set(GLM_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glm")

# --- 4. STB_IMAGE (Header Only) ---
# 同样只需要头文件路径
set(STB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb_image/include")

# --- 5. ImGui (构建为静态库) ---
# 收集 ImGui 目录下所有的 .cpp 文件
file(GLOB IMGUI_SOURCES "third_party/imgui/*.cpp")
add_library(imgui STATIC ${IMGUI_SOURCES})
# ImGui 需要包含 GLFW 和 OpenGL 的头文件才能编译 backends
target_include_directories(imgui PUBLIC 
    "third_party/imgui" 
    ${GLFW_INCLUDE_DIR}
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include" # ImGui opengl3 backend 需要 glad
)

# --- 6. Assimp (外部链接库) ---
set(ASSIMP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/assimp/include")
set(ASSIMP_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/assimp/lib")
# 查找库文件
find_library(ASSIMP_LIB_REL NAMES assimp-vc143-mt assimp PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
find_library(ASSIMP_LIB_DBG NAMES assimp-vc143-mtd assimp-d PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
# 2. 【新增】查找 Zlib 库
# Assimp 静态编译依赖 zlibstatic
find_library(ZLIB_LIB NAMES zlibstatic zlib zlibstaticd PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
# 3. 【可选】查找 IrrXML 库 (如果后续报 irrXML 相关的错，把这一行加上)
# find_library(IRRXML_LIB NAMES irrxml PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)

# ==========================================
# 2. 配置资源文件 (Assets)
# ==========================================
# 只有当 assets 文件夹存在时才复制，避免报错
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION "${CMAKE_BINARY_DIR}")
endif()

# 同时需要把 Assimp 的 DLL (如果有) 拷贝到运行目录
# 因为这里是静态编译 Assimp，通常不需要额外的 DLL 复制步骤了，所以下面的配置可以省略
file(GLOB ASSIMP_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/third_party/assimp/lib/*.dll")
if(ASSIMP_DLLS)
    file(COPY ${ASSIMP_DLLS} DESTINATION "${CMAKE_BINARY_DIR}")
endif()

# ==========================================
# 3. 添加子目录 src
# ==========================================
add_subdirectory(src)

# ==========================================
# 4. 配置主程序 (Main Executable)
# ==========================================
add_executable(glMain main.cpp)

# ==========================================
# 5. 链接库 (Linking)
# ==========================================
# 链接顺序：
# 1. CoreLib (src下的代码)
# 2. glad (刚才定义的库)
# 3. GLFW (导入的库)
# 4. opengl32 (Windows 系统自带库，必须链接)
target_link_libraries(glMain 
    PRIVATE 
    CoreLib 
    glad 
    imgui
    ${GLFW_LIB}
    debug ${ASSIMP_LIB_DBG}       # Debug 模式用这个
    optimized ${ASSIMP_LIB_REL}   # Release 模式用这个
    ${ZLIB_LIB}   # 【新增】Assimp 的跟班 Zlib
    opengl32 
)

# 统一包含路径 (让 main.cpp 能找到所有头文件)
target_include_directories(glMain PRIVATE 
    ${GLFW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}    
    ${STB_INCLUDE_DIR}    
    ${ASSIMP_INCLUDE_DIR} 
)



# ==========================================
#  配置测试程序 (Test Executable)
# ==========================================
add_executable(glTest test.cpp)

# ==========================================
# 链接库 (Linking)
# ==========================================
# 链接顺序：
# 1. CoreLib (src下的代码)
# 2. glad (刚才定义的库)
# 3. GLFW (导入的库)
# 4. opengl32 (Windows 系统自带库，必须链接)
target_link_libraries(glTest  
    PRIVATE 
    CoreLib 
    glad 
    imgui
    ${GLFW_LIB}
    debug ${ASSIMP_LIB_DBG}       # Debug 模式用这个
    optimized ${ASSIMP_LIB_REL}   # Release 模式用这个
    ${ZLIB_LIB}   # 【新增】Assimp 的跟班 Zlib
    opengl32 
)

# 统一包含路径 (让 main.cpp 能找到所有头文件)
target_include_directories(glTest PRIVATE 
    ${GLFW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}    
    ${STB_INCLUDE_DIR}    
    ${ASSIMP_INCLUDE_DIR} 
)