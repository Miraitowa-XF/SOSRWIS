# 需求的最低cmake程序版本
cmake_minimum_required(VERSION 3.12)

# 本工程的名字
project(SOSRWIS)

# 本工程支持的C++版本
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # 强制要求，不支持就报错
set(CMAKE_CXX_EXTENSIONS OFF)        # 只使用标准 C++，不使用编译器扩展

# ==========================================
# 1. 配置第三方库 (Third Party Setup)
# ==========================================

# --- 配置 GLAD ---
# 将 glad.c 编译成一个静态库，名字叫 glad
add_library(glad STATIC "third_party/glad/src/glad.c")
# 告诉 glad 库，它的头文件在哪里
target_include_directories(glad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include")

# --- 配置 GLFW ---
# 首先在系统路径中查找 GLFW
find_package(glfw3 QUIET)

if(glfw3_FOUND)
    # 使用系统安装的 GLFW
    message(STATUS "Found GLFW in system path")
    # 为了避免 CMake 报错，设置这些变量为空
    set(GLFW_LIB "")
else()
    # 如果系统中找不到，在 third_party 目录中查找
    message(STATUS "GLFW not found in system path, searching in third_party")
    set(GLFW_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include")
    set(GLFW_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/lib")
    if(WIN32)
        find_library(GLFW_LIB NAMES glfw3 PATHS ${GLFW_LIB_DIR} NO_DEFAULT_PATH)
    elseif(APPLE)
        find_library(GLFW_LIB NAMES glfw3 libglfw3.dylib libglfw3.a PATHS ${GLFW_LIB_DIR} NO_DEFAULT_PATH)
    else()
        find_library(GLFW_LIB NAMES glfw3 libglfw3.so PATHS ${GLFW_LIB_DIR} NO_DEFAULT_PATH)
    endif()
endif()

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
    $<IF:$<TARGET_EXISTS:glfw>,,$<IF:$<NOT:$<STREQUAL:${GLFW_INCLUDE_DIR},>>,${GLFW_INCLUDE_DIR},>>
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include" # ImGui opengl3 backend 需要 glad
)

# 如果找到了系统安装的 GLFW，链接它
target_link_libraries(imgui PRIVATE $<IF:$<TARGET_EXISTS:glfw>,glfw,${GLFW_LIB}>)

# --- 6. Assimp (外部链接库) ---
# 首先在系统路径中查找 Assimp
find_package(assimp QUIET)

if(assimp_FOUND)
    # 使用系统安装的 Assimp
    message(STATUS "Found Assimp in system path")
    # 为了避免 CMake 报错，设置这些变量为空
    set(ASSIMP_LIB_REL "")
    set(ASSIMP_LIB_DBG "")
else()
    # 如果系统中找不到，在 third_party 目录中查找
    message(STATUS "Assimp not found in system path, searching in third_party")
    set(ASSIMP_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/assimp/include")
    set(ASSIMP_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/assimp/lib")
    if(WIN32)
        find_library(ASSIMP_LIB_REL NAMES assimp-vc143-mt assimp PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
        find_library(ASSIMP_LIB_DBG NAMES assimp-vc143-mtd assimp-d PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
    elseif(APPLE)
        find_library(ASSIMP_LIB_REL NAMES assimp libassimp.dylib libassimp.a PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
        find_library(ASSIMP_LIB_DBG NAMES assimp libassimp.dylib libassimp.a PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
    else()
        find_library(ASSIMP_LIB_REL NAMES assimp libassimp.so PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
        find_library(ASSIMP_LIB_DBG NAMES assimp libassimp.so PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
    endif()
endif()

# 查找 Zlib 库
find_package(ZLIB QUIET)

if(ZLIB_FOUND)
    # 使用系统安装的 Zlib
    message(STATUS "Found Zlib in system path")
    # 为了避免 CMake 报错，设置这些变量为空
    set(ZLIB_LIB "")
else()
    # 如果系统中找不到，在 third_party 目录中查找
    message(STATUS "Zlib not found in system path, searching in third_party")
    set(ASSIMP_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/assimp/lib")
    if(WIN32)
        find_library(ZLIB_LIB NAMES zlibstatic zlib zlibstaticd PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
    elseif(APPLE)
        find_library(ZLIB_LIB NAMES zlib libz.dylib libz.a PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
    else()
        find_library(ZLIB_LIB NAMES zlib libz.so PATHS ${ASSIMP_LIB_DIR} NO_DEFAULT_PATH)
    endif()
endif()
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
# 设置要链接的库
set(LIBS_TO_LINK 
    CoreLib 
    glad 
    imgui
)

# 添加 GLFW
if(TARGET glfw)
    list(APPEND LIBS_TO_LINK glfw)
else()
    list(APPEND LIBS_TO_LINK ${GLFW_LIB})
endif()

# 添加 Assimp
if(TARGET assimp)
    list(APPEND LIBS_TO_LINK assimp)
    message(STATUS "Linking with system Assimp library")
elseif(NOT "${ASSIMP_LIB_REL}" STREQUAL "")
    list(APPEND LIBS_TO_LINK debug ${ASSIMP_LIB_DBG} optimized ${ASSIMP_LIB_REL})
    message(STATUS "Linking with local Assimp library: ${ASSIMP_LIB_REL}")
else()
    # 尝试直接链接 assimp 库
    find_library(ASSIMP_LIBRARY NAMES assimp libassimp.dylib libassimp.a)
    if(ASSIMP_LIBRARY)
        list(APPEND LIBS_TO_LINK ${ASSIMP_LIBRARY})
        message(STATUS "Linking with found Assimp library: ${ASSIMP_LIBRARY}")
    else()
        message(FATAL_ERROR "Could not find Assimp library")
    endif()
endif()

# 添加 Zlib
if(TARGET ZLIB::ZLIB)
    list(APPEND LIBS_TO_LINK ZLIB::ZLIB)
else()
    list(APPEND LIBS_TO_LINK ${ZLIB_LIB})
endif()

# 添加平台特定的 OpenGL 库
if(WIN32)
    list(APPEND LIBS_TO_LINK opengl32)
elseif(APPLE)
    list(APPEND LIBS_TO_LINK "-framework OpenGL")
else()
    list(APPEND LIBS_TO_LINK GL)
endif()

# 链接库
target_link_libraries(glMain PRIVATE ${LIBS_TO_LINK})

# 统一包含路径 (让 main.cpp 能找到所有头文件)
target_include_directories(glMain PRIVATE 
    ${GLM_INCLUDE_DIR}    
    ${STB_INCLUDE_DIR}    
)

# 只有在没有使用系统安装的库时，才需要指定第三方库的 include 目录
if(NOT glfw3_FOUND AND NOT TARGET glfw)
    target_include_directories(glMain PRIVATE ${GLFW_INCLUDE_DIR})
endif()

if(NOT assimp_FOUND AND NOT TARGET assimp)
    target_include_directories(glMain PRIVATE ${ASSIMP_INCLUDE_DIR})
endif()



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
# 链接库
target_link_libraries(glTest PRIVATE ${LIBS_TO_LINK})

# 统一包含路径 (让 main.cpp 能找到所有头文件)
target_include_directories(glTest PRIVATE 
    ${GLM_INCLUDE_DIR}    
    ${STB_INCLUDE_DIR}    
)

# 只有在没有使用系统安装的库时，才需要指定第三方库的 include 目录
if(NOT glfw3_FOUND AND NOT TARGET glfw)
    target_include_directories(glTest PRIVATE ${GLFW_INCLUDE_DIR})
endif()

if(NOT assimp_FOUND AND NOT TARGET assimp)
    target_include_directories(glTest PRIVATE ${ASSIMP_INCLUDE_DIR})
endif()