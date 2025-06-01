#version 330 core

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    vec2 TexCoords;
    vec4 FragPosSunLight;
    vec4 FragPosChandelierLights[6];
} fs_in;

uniform sampler2D texture1;
uniform sampler2D texture2; // Normal map
uniform sampler2D sunShadowMap;
uniform sampler2D chandelierShadowMaps[3]; // Limited to 3 for texture units

// Lighting uniforms
uniform vec3 viewPos;
uniform int numLights;
uniform vec3 lightPositions[6];
uniform float lightIntensity;
uniform float normalMapStrength;

// Enhanced sun uniforms
uniform vec3 sunPosition;
uniform float sunIntensity;
uniform vec3 sunColor;
uniform float timeOfDay;

out vec4 FragColor;

// Enhanced shadow calculation with PCF (Percentage-Closer Filtering)
float calculateShadow(sampler2D shadowMap, vec4 fragPosLight, vec3 normal, vec3 lightDir) {
    // Perspective divide
    vec3 projCoords = fragPosLight.xyz / fragPosLight.w;
    
    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    
    // Check if fragment is outside shadow map
    if(projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || 
       projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0; // No shadow
    }
    
    // Calculate bias to prevent shadow acne
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    
    // PCF (Percentage-Closer Filtering) for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    
    // 3x3 PCF sampling
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            vec2 offset = vec2(x, y) * texelSize;
            float pcfDepth = texture(shadowMap, projCoords.xy + offset).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0; // Average of 9 samples
    
    return shadow;
}

// Enhanced lighting calculation with shadows
vec3 calculateLighting(vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo) {
    vec3 ambient = vec3(0.1, 0.1, 0.15); // Slightly blue ambient
    vec3 result = ambient * albedo;
    
    // Point lights (chandelier) with shadows
    for(int i = 0; i < numLights; i++) {
        vec3 lightDir = normalize(lightPositions[i] - fragPos);
        float distance = length(lightPositions[i] - fragPos);
        
        // Enhanced attenuation for more realistic falloff
        float attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        
        // Calculate shadow for this light (limit to first 3 lights for shadow maps)
        float shadow = 0.0;
        if(i < 3) {
            shadow = calculateShadow(chandelierShadowMaps[i], fs_in.FragPosChandelierLights[i], 
                                   normal, lightDir);
        }
        
        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(1.0, 0.9, 0.7) * lightIntensity; // Warm white light
        
        // Specular
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
        vec3 specular = spec * vec3(1.0, 0.9, 0.7) * lightIntensity * 0.5;
        
        // Apply shadow (reduce lighting where in shadow)
        float shadowFactor = 1.0 - shadow * 0.8; // 80% shadow intensity
        result += (diffuse + specular) * attenuation * albedo * shadowFactor;
    }
    
    // Enhanced sun lighting with shadows
    if(sunIntensity > 0.0) {
        vec3 sunDir = normalize(sunPosition - fragPos);
        float sunDistance = length(sunPosition - fragPos);
        
        // Calculate sun shadow
        float sunShadow = calculateShadow(sunShadowMap, fs_in.FragPosSunLight, normal, sunDir);
        
        // More realistic sun attenuation (less falloff for distant light)
        float sunAttenuation = 1.0 / (1.0 + 0.001 * sunDistance);
        
        // Sun diffuse with color temperature
        float sunDiff = max(dot(normal, sunDir), 0.0);
        vec3 sunDiffuse = sunDiff * sunColor * sunIntensity;
        
        // Sun specular
        vec3 sunReflectDir = reflect(-sunDir, normal);
        float sunSpec = pow(max(dot(viewDir, sunReflectDir), 0.0), 32.0);
        vec3 sunSpecular = sunSpec * sunColor * sunIntensity * 0.3;
        
        // Apply sun shadow (stronger shadow effect for sun)
        float sunShadowFactor = 1.0 - sunShadow * 0.9; // 90% shadow intensity
        
        result += (sunDiffuse + sunSpecular) * sunAttenuation * albedo * sunShadowFactor;
    }
    
    // Enhanced ambient based on time of day
    vec3 timeAmbient = vec3(0.05);
    if(timeOfDay >= 5.0 && timeOfDay <= 22.0) {
        // Dawn/dusk ambience
        if(timeOfDay < 6.0 || timeOfDay > 21.0) {
            timeAmbient = mix(vec3(0.02, 0.02, 0.05), vec3(0.1, 0.05, 0.02), 
                             sin((timeOfDay - 5.0) / 2.0 * 3.14159));
        } else {
            // Day ambience
            float dayFactor = smoothstep(6.0, 12.0, timeOfDay) - smoothstep(15.0, 21.0, timeOfDay);
            timeAmbient = mix(vec3(0.05, 0.05, 0.08), vec3(0.15, 0.15, 0.2), dayFactor);
        }
    } else {
        // Night ambience
        timeAmbient = vec3(0.01, 0.01, 0.03);
    }
    
    result += timeAmbient * albedo;
    
    return result;
}

void main() {
    vec3 albedo = texture(texture1, fs_in.TexCoords).rgb;
    
    // Enhanced normal mapping
    vec3 normalMap = texture(texture2, fs_in.TexCoords).rgb * 2.0 - 1.0;
    normalMap.xy *= normalMapStrength;
    
    // Create TBN matrix for normal mapping
    vec3 T = normalize(fs_in.Tangent);
    vec3 B = normalize(fs_in.Bitangent);
    vec3 N = normalize(fs_in.Normal);
    mat3 TBN = mat3(T, B, N);
    
    vec3 normal = normalize(TBN * normalMap);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    
    // Calculate enhanced lighting with shadows
    vec3 color = calculateLighting(normal, fs_in.FragPos, viewDir, albedo);
    
    // Enhanced tone mapping for more realistic exposure
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2)); // Gamma correction
    
    FragColor = vec4(color, 1.0);
}