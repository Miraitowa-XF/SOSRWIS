#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// 贴图采样器
uniform sampler2D texture_diffuse1;

// 光照参数
uniform vec3 lightPos;  // 光源位置
uniform vec3 viewPos;   // 摄像机位置
uniform vec3 lightColor;// 光的颜色

void main()
{
    // 1. 获取纹理颜色 (包含 RGBA)
    vec4 texColor = texture(texture_diffuse1, TexCoords);

    // 【透明度测试】
    // 如果是完全透明的部分（比如玻璃上的镂空、草丛边缘），直接丢弃
    if(texColor.a < 0.1)
        discard;

    vec3 objectColor = texColor.rgb;

    // ---------------------------------------------------------
    // 【新增】模拟自发光逻辑 (让灯泡亮起来)
    // ---------------------------------------------------------
    // 计算当前像素的亮度 (Luminance)。人眼对绿色最敏感，所以系数不同。
    float brightness = dot(objectColor, vec3(0.2126, 0.7152, 0.0722));

    // 阈值判断：如果亮度超过 0.9 (接近纯白)，我们认为它是发光体（如灯泡）
    if(brightness > 0.9)
    {
        // 直接输出原色！不让它受环境光和阴影的影响，从而看起来像在发光
        FragColor = vec4(objectColor, texColor.a);
    }
    else
    {
        // ---------------------------------------------------------
        // 对于普通物体，执行标准的冯氏光照计算
        // ---------------------------------------------------------
        
        // 1. 环境光 (Ambient)
        float ambientStrength = 0.5; // 稍微调亮一点，防止阴影处太黑
        vec3 ambient = ambientStrength * lightColor;
    
        // 2. 漫反射 (Diffuse)
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        
        // 3. 镜面反射 (Specular)
        float specularStrength = 0.5;
        vec3 viewDir = normalize(viewPos - FragPos);
        vec3 reflectDir = reflect(-lightDir, norm);  
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
        vec3 specular = specularStrength * spec * lightColor;  
        
        // 4. 合并结果
        vec3 result = (ambient + diffuse + specular) * objectColor;

        // 输出最终颜色
        FragColor = vec4(result, texColor.a);
    }
}