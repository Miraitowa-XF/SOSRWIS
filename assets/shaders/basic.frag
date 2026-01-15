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

// --- 【新增】路灯 (点光源) ---
#define NR_POINT_LIGHTS 4  // 定义路灯数量：4盏
struct PointLight {
    vec3 position;
    vec3 color;
    
    float constant;
    float linear;
    float quadratic;
};
uniform PointLight pointLights[NR_POINT_LIGHTS]; // 数组
uniform bool lampOn;     // 开关状态

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

// 新增：计算单个点光源的函数
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 objectColor)
{
    vec3 lightDir = normalize(light.position - fragPos);
    
    // 漫反射
    float diff = max(dot(normal, lightDir), 0.0);
    // 镜面反射
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    
    // 衰减
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));    

    // 合并
    vec3 diffuse = light.color * diff * objectColor;
    vec3 specular = light.color * spec * 0.5; // 0.5是镜面强度
    
    diffuse *= attenuation;
    specular *= attenuation;
    
    return (diffuse + specular);
}

void main()
{
    vec4 texColor = texture(texture_diffuse1, TexCoords);
    if(texColor.a < 0.01) discard;
    vec3 objectColor = texColor.rgb;
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // =======================================================
    // 1. 计算太阳/月亮光照 (基础环境光在这里计算)
    // =======================================================
    vec3 sunLightDir = normalize(lightPos - FragPos);
    
    // 【修复 1】直接使用传入的 ambientStrength，不再乘以 0.4
    // 这样 SunSystem 里的 0.03 (深夜) 就会生效，黑夜会变得很黑
    vec3 ambient = ambientStrength * lightColor; 
    
    float diff = max(dot(norm, sunLightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    vec3 reflectDir = reflect(-sunLightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = 0.2 * spec * lightColor; 

    float shadow = ShadowCalculation(FragPosLightSpace, norm, sunLightDir);       
    vec3 result = (ambient + (1.0 - shadow) * (diffuse + specular)) * objectColor;

    // =======================================================
    // 2. 【升级】计算 4 盏路灯
    // =======================================================
    if(lampOn) // 总开关
    {
        for(int i = 0; i < NR_POINT_LIGHTS; i++)
        {
            // 叠加每一盏灯的光照贡献
            result += CalcPointLight(pointLights[i], norm, FragPos, viewDir, objectColor);
        }
    }

    FragColor = vec4(result, texColor.a);
}