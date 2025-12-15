// 启用 stb_image 的实现 (必须在 include 之前定义，且在一个项目中只能定义一次)
// 这样在其他文件中只写 include，不要写 define，以避免重复定义错误
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"