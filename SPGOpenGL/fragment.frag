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
uniform float    lightIntensity;

// Adãugãm uniforms pentru lumina soarelui
uniform vec3     sunPosition;
uniform float    sunIntensity;
uniform float    timeOfDay;

vec3 calculateLighting(vec3 lightPos, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color) {
    float distance = length(lightPos - fragPos);
    float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    
    // Fãrã ambient pentru candelabru
    vec3 ambient = vec3(0.0);
    
    vec3 lightDir = normalize(lightPos - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * color * 0.8;
    
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = vec3(0.2) * spec;
    
    return (ambient + diffuse + specular) * attenuation;
}

vec3 calculateSunLighting(vec3 normal, vec3 fragPos, vec3 viewDir, vec3 color) {
    if (sunIntensity <= 0.0) return vec3(0.0);
    
    // Calculeazã lumina naturalã bazatã pe timpul din zi
    float naturalLight = 0.0;
    if (timeOfDay >= 6.0 && timeOfDay <= 18.0) {
        if (timeOfDay >= 10.0 && timeOfDay <= 14.0) {
            naturalLight = 1.0;
        } else if (timeOfDay < 10.0) {
            naturalLight = (timeOfDay - 6.0) / 4.0;
        } else {
            naturalLight = (18.0 - timeOfDay) / 4.0;
        }
    }
    
    naturalLight *= sunIntensity;
    if (naturalLight <= 0.0) return vec3(0.0);
    
    vec3 sunDir = normalize(sunPosition - fragPos);
    float sunDistance = length(sunPosition - fragPos);
    
    // Atenuare pentru soare (mai pu?in decât pentru candelabru)
    float sunAttenuation = 1.0 / (1.0 + 0.01 * sunDistance + 0.001 * sunDistance * sunDistance);
    
    // Lumina solarã difuzã
    float diff = max(dot(normal, sunDir), 0.0);
    vec3 sunDiffuse = diff * color * naturalLight * sunAttenuation;
    
    // Culoarea soarelui în func?ie de timp
    vec3 sunColor = vec3(1.0, 0.9, 0.7);
    if (timeOfDay >= 5.0 && timeOfDay <= 8.0) {
        float morning = clamp((timeOfDay - 5.0) / 3.0, 0.0, 1.0);
        sunColor = mix(vec3(1.0, 0.4, 0.1), vec3(1.0, 1.0, 0.9), morning);
    } else if (timeOfDay >= 17.0 && timeOfDay <= 19.0) {
        float evening = clamp((timeOfDay - 17.0) / 2.0, 0.0, 1.0);
        sunColor = mix(vec3(1.0, 1.0, 0.9), vec3(1.0, 0.2, 0.05), evening);
    }
    
    return sunDiffuse * sunColor;
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
    
    // Ini?ializeazã rezultatul cu întuneric complet
    vec3 result = vec3(0.0);
    
    // Adaugã lumina candelabrului doar dacã e activ
    if (numLights > 0 && lightIntensity > 0.0) {
        for(int i = 0; i < numLights && i < 6; i++) {
            result += calculateLighting(lightPositions[i], finalNormal, fs.FragPos, viewDir, color);
        }
        result *= lightIntensity;
    }
    
    // Adaugã lumina soarelui
    result += calculateSunLighting(finalNormal, fs.FragPos, viewDir, color);
    
    // FÃRÃ ambient - întuneric complet când nu sunt lumini
    
    FragColor = vec4(result, 1.0);
}