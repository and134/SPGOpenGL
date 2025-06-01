#version 330 core

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture1;
uniform sampler2D texture2;

uniform vec3 viewPos;
uniform int numLights;
uniform vec3 lightPositions[6];
uniform float lightIntensity;
uniform float normalMapStrength;

uniform vec3 sunPosition;
uniform float sunIntensity;
uniform vec3 sunColor;
uniform float timeOfDay;

out vec4 FragColor;

float calculateObjectShadow(vec3 fragPos, vec3 lightPos, vec3 normal) {
    vec3 lightDir = normalize(lightPos - fragPos);
    float lightDistance = length(lightPos - fragPos);
    float angleFactor = dot(normal, lightDir);
    
    float shadowFactor = 1.0;
    
    if (fragPos.y < -0.7) {
        if (lightDistance > 4.0) {
            shadowFactor = 0.15;
        } else if (lightDistance > 2.5) {
            shadowFactor = 0.25;
        } else {
            shadowFactor = 0.4;
        }
    }
    
    if (angleFactor < 0.1) {
        shadowFactor = 0.1;
    } else if (angleFactor < 0.3) {
        shadowFactor = 0.3; 
    } else if (angleFactor < 0.6) {
        shadowFactor = 0.6;
    }
    
    if (lightDistance > 6.0) {
        shadowFactor *= 0.5;
    } else if (lightDistance > 4.0) {
        shadowFactor *= 0.7;
    }
    return shadowFactor;
}

float calculateSunShadow(vec3 fragPos, vec3 sunPos, vec3 normal, float timeOfDay) {
    vec3 sunDir = normalize(sunPos - fragPos);
    float sunDistance = length(sunPos - fragPos);
    float angleFactor = dot(normal, sunDir);
    
    float shadowFactor = 1.0;
    float sunHeight = sunPos.y;
    
    if (fragPos.y < -0.7 && sunHeight < 3.0) {
        if (timeOfDay < 8.0 || timeOfDay > 18.0) {
            shadowFactor = 0.1;
        } else if (timeOfDay < 10.0 || timeOfDay > 16.0) {
            shadowFactor = 0.2;
        } else {
            shadowFactor = 0.4;
        }
    }
    
    if (angleFactor < 0.2) {
        shadowFactor = 0.05; 
    } else if (angleFactor < 0.5) {
        shadowFactor = 0.3;
    }
    
    float sunZ = sunPos.z;
    if (sunZ > 0 && fragPos.z < 0) {
        shadowFactor *= 0.3;
    } else if (sunZ < -5 && fragPos.z > -5) {
        shadowFactor *= 0.4;
    }
    return shadowFactor;
}

vec3 calculateLighting(vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo) {
    vec3 ambient = vec3(0.05, 0.05, 0.08);
    vec3 result = ambient * albedo;
   
    for(int i = 0; i < numLights; i++) {
        vec3 lightDir = normalize(lightPositions[i] - fragPos);
        float distance = length(lightPositions[i] - fragPos);
        
        float attenuation = 1.0 / (1.0 + 0.14 * distance + 0.07 * distance * distance);
       
        float shadowFactor = calculateObjectShadow(fragPos, lightPositions[i], normal);
        
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(1.0, 0.9, 0.7) * lightIntensity;
        
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
        vec3 specular = spec * vec3(1.0, 0.9, 0.7) * lightIntensity * 0.5;
        
        result += (diffuse + specular) * attenuation * albedo * shadowFactor;
    }
    
    if(sunIntensity > 0.0) {
        vec3 sunDir = normalize(sunPosition - fragPos);
        float sunDistance = length(sunPosition - fragPos);
        
        float sunShadowFactor = calculateSunShadow(fragPos, sunPosition, normal, timeOfDay);
        
        float sunAttenuation = 1.0 / (1.0 + 0.002 * sunDistance);
        float clampedSunIntensity = min(sunIntensity, 2.0);
        
        float sunDiff = max(dot(normal, sunDir), 0.0);
        vec3 sunDiffuse = sunDiff * sunColor * clampedSunIntensity;
        
        vec3 sunReflectDir = reflect(-sunDir, normal);
        float sunSpec = pow(max(dot(viewDir, sunReflectDir), 0.0), 32.0);
        vec3 sunSpecular = sunSpec * sunColor * clampedSunIntensity * 0.2;
        
        vec3 sunContribution = (sunDiffuse + sunSpecular) * sunAttenuation * sunShadowFactor;
        sunContribution = min(sunContribution, vec3(1.5)); 
        
        result += sunContribution * albedo;
    }
    
    vec3 timeAmbient = vec3(0.02);
    if(timeOfDay >= 5.0 && timeOfDay <= 22.0) {
        if(timeOfDay < 6.0 || timeOfDay > 21.0) {
            timeAmbient = mix(vec3(0.01, 0.01, 0.03), vec3(0.08, 0.04, 0.02), sin((timeOfDay - 5.0) / 2.0 * 3.14159));
        } else {
            float dayFactor = smoothstep(6.0, 12.0, timeOfDay) - smoothstep(15.0, 21.0, timeOfDay);
            timeAmbient = mix(vec3(0.03, 0.03, 0.05), vec3(0.08, 0.08, 0.1), dayFactor);
        }
    } else {
        timeAmbient = vec3(0.005, 0.005, 0.02);
    }
    
    result += timeAmbient * albedo;
    result = min(result, vec3(2.0));
    
    return result;
}

void main() {
    vec3 albedo = texture(texture1, fs_in.TexCoords).rgb;
    
    vec3 normalMap = texture(texture2, fs_in.TexCoords).rgb * 2.0 - 1.0;
    normalMap.xy *= min(normalMapStrength, 0.8);
    
    vec3 T = normalize(fs_in.Tangent);
    vec3 B = normalize(fs_in.Bitangent);
    vec3 N = normalize(fs_in.Normal);
    
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    
    vec3 normal = normalize(TBN * normalMap);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    
    vec3 color = calculateLighting(normal, fs_in.FragPos, viewDir, albedo);
    
    color = color / (color + vec3(0.8));
    color = pow(color, vec3(1.0/2.2));
    
    color = clamp(color, vec3(0.0), vec3(1.0));
    
    FragColor = vec4(color, 1.0);
}