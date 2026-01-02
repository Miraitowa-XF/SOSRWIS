#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
// 【新增】接收光空间坐标（用于查阴影图）
in vec4 FragPosLightSpace; 

// 纹理采样器
uniform sampler2D texture_diffuse1;
// 【新增】深度贴图（阴影图）
uniform sampler2D shadowMap; 

// 动态太阳参数
uniform vec3 lightPos;           // 太阳光的方向向量
uniform vec3 lightColor;         // 太阳的实时颜色
uniform float sunIntensity;    // 太阳的实时强度
uniform float ambientStrength; // 实时环境光
uniform vec3 viewPos;

// ==========================================================
// 阴影计算函数
// 返回值: 1.0 表示在阴影中(全黑)，0.0 表示不在阴影中(亮)
// ==========================================================
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
    // 1. 执行透视除法 (将坐标变换到 [-1,1] 范围)
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    
    // 2. 变换到 [0,1] 的范围 (因为深度图是 0 到 1)
    projCoords = projCoords * 0.5 + 0.5;
    
    // 3. 如果超过了深度图的范围(比如远处的地面)，就不计算阴影，设为 0
    if(projCoords.z > 1.0)
        return 0.0;
        
    // 4. 获取当前片段的深度 (当前像素距离光源的距离)
    float currentDepth = projCoords.z;
    
    // 5. 计算偏置 (Shadow Bias) - 非常重要！解决“阴影波纹(Acne)”
    // 根据光线角度动态调整偏置量
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);  

    // 6. PCF (Percentage-Closer Filtering) 柔化阴影
    // 采样周围 9 个点取平均值，让阴影边缘不那么锯齿
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0); // 计算单个纹理像素的大小
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            // 读取深度图上周围像素的深度值
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            // 比较深度：如果当前深度 > 记录的深度，说明被挡住了
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0; // 取平均
    
    return shadow;
}

void main()
{
    // 1. 获取纹理颜色
    vec4 texColor = texture(texture_diffuse1, TexCoords);

    // 【透明度测试】直接丢弃透明像素 (草丛、玻璃孔洞等)
    if(texColor.a < 0.1)
        discard;

    vec3 objectColor = texColor.rgb;

    // ==========================================================
    // 标准冯氏光照计算 (Phong Lighting)
    // ==========================================================

    // 2. 环境光 (Ambient) 
    // 这是不管有没有影子都能照亮的部分
    // float ambientStrength = 0.4; 
    vec3 ambient = ambientStrength * lightColor;
  
    // 3. 漫反射 (Diffuse)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * sunIntensity * lightColor;
    
    // 4. 镜面反射 (Specular)
    float specularStrength = 0.2; // 稍微调低一点，避免塑料感太强
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32); 
    vec3 specular = specularStrength * spec * sunIntensity * lightColor;
    
    // ==========================================================
    // 阴影应用
    // ==========================================================
    
    // 计算阴影因子 (0.0 = 无阴影, 1.0 = 全阴影)
    float shadow = ShadowCalculation(FragPosLightSpace, norm, lightDir);       
    
    // 最终颜色公式：
    // Ambient 不受阴影影响 (否则阴影里就是死黑)
    // Diffuse 和 Specular 会被阴影遮挡
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * objectColor;    
    
    FragColor = vec4(lighting, texColor.a);
}