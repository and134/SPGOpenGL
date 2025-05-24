#version 330 core
in VS_OUT {
    vec3  FragPos;
    vec3  Normal;
    vec3  Tangent;
    vec3  Bitangent;
    vec2  TexCoords;
} fs;
out vec4 FragColor;

uniform sampler2D texture1;    // diffuse
uniform sampler2D texture2;    // normal
uniform vec3     lightPositions[6]; // Multiple light sources
uniform int      numLights;
uniform vec3     viewPos;
uniform float    normalMapStrength;
uniform float lightIntensity; 

vec3 calculateLighting(vec3 lightPos, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color) {
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    // Reduce ambient significantly
    vec3 ambient = 0.05 * color;  // Changed from 0.15 to 0.05
    
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * color * 0.8;  // Scale down diffuse
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = vec3(0.2) * spec;  // Reduce specular from 0.5 to 0.2
    
    return (ambient + diffuse + specular) * attenuation;
}

void main(){
    // Sample textures
    vec3 color = texture(texture1, fs.TexCoords).rgb;
    vec3 normalMap = texture(texture2, fs.TexCoords).rgb;
    
    // Transform normal from tangent space to world space
    vec3 normal = normalize(fs.Normal);
    vec3 tangent = normalize(fs.Tangent);
    vec3 bitangent = normalize(fs.Bitangent);
    
    // Create TBN matrix
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // Transform normal map from [0,1] to [-1,1] and apply to world space
    vec3 normalFromMap = normalize(normalMap * 2.0 - 1.0);
    vec3 finalNormal = normalize(TBN * mix(vec3(0,0,1), normalFromMap, normalMapStrength));
    
    // View direction
    vec3 viewDir = normalize(viewPos - fs.FragPos);
    
    // Calculate lighting from all light sources
    vec3 result = vec3(0.0);
    
    for(int i = 0; i < numLights && i < 6; i++) {
        result += calculateLighting(lightPositions[i], finalNormal, fs.FragPos, viewDir, color);
    }
    
    // Add some global ambient
    result *= lightIntensity;
    
    FragColor = vec4(result, 1.0);
}