#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    // 移除 view 矩阵的位移部分，让天空盒永远跟着相机跑
    vec4 pos = projection * mat4(mat3(view)) * vec4(aPos, 1.0);
    
    // 技巧：让 z 总是等于 w，这样透视除法后 z=1.0 (深度最大值)
    gl_Position = pos.xyww; 
}