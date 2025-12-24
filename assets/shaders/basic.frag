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
    // 1. 环境光 (Ambient)
    float ambientStrength = 0.3; // 稍微调亮一点，防止阴影处死黑
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
    
    // 4. 采样纹理颜色
    vec3 objectColor = texture(texture_diffuse1, TexCoords).rgb;

    // 【修改后】强制使用一个灰白色 (先让模型显形！测试光照等的设置是否正确)
    // vec3 objectColor = vec3(0.8, 0.8, 0.8); 

    // 5. 合并结果
    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}