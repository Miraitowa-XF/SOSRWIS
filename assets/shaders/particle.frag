#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
uniform sampler2D particleTexture; // 雪花的图片（png透明背景）

void main()
{
    vec4 texColor = texture(particleTexture, TexCoords);
    
    // 关键：透明度测试
    // 如果像素太透明（雪花的透明背景部分），直接丢弃，不渲染
    if(texColor.a < 0.2)
        discard;
        
    // 可以在这里给雪花染上一点环境光的颜色，或者直接白色
    //FragColor = texColor;
    FragColor = texColor * vec4(1.2, 1.2, 1.2, 1.0); // 稍微提亮一点
}