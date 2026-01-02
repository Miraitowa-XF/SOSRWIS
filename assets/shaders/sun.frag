#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 sunColor;
uniform vec3 viewPos;
uniform float sunIntensity;

void main()
{
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 norm = normalize(Normal);

    // 计算中心发光感
    float centerFactor = max(dot(norm, viewDir), 0.0);
    float glow = pow(centerFactor, 4.0); 
    
    // 颜色混合：中心亮白色，边缘颜色
    vec3 finalColor = mix(sunColor, vec3(1.1, 1.1, 1.0), glow);

    // 最后的亮度增益
    finalColor *= sunIntensity;

    FragColor = vec4(finalColor, 1.0);
}