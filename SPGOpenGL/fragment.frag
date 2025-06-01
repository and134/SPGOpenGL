#version 330 core

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec3 Tangent;
    vec3 Bitangent;
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture1;
uniform sampler2D texture2; // Normal map

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

// Enhanced shadow calculation with much more obvious shadows
float calculateObjectShadow(vec3 fragPos, vec3 lightPos, vec3 normal) {
    vec3 lightDir = normalize(lightPos - fragPos);
    float lightDistance = length(lightPos - fragPos);
    float angleFactor = dot(normal, lightDir);
    
    float shadowFactor = 1.0;
    
    // MUCH stronger shadows under objects and in corners
    if (fragPos.y < -0.7) {
        // Floor shadows - very obvious
        if (lightDistance > 4.0) {
            shadowFactor = 0.15; // Very dark shadow
        } else if (lightDistance > 2.5) {
            shadowFactor = 0.25; // Strong shadow
        } else {
            shadowFactor = 0.4;  // Medium shadow
        }
    }
    
    // Wall shadows based on angle
    if (angleFactor < 0.1) {
        shadowFactor = 0.1; // Very dark for surfaces facing away
    } else if (angleFactor < 0.3) {
        shadowFactor = 0.3; // Dark shadow for steep angles
    } else if (angleFactor < 0.6) {
        shadowFactor = 0.6; // Medium shadow
    }
    
    // Distance-based shadows
    if (lightDistance > 6.0) {
        shadowFactor *= 0.5; // Additional darkening for distance
    } else if (lightDistance > 4.0) {
        shadowFactor *= 0.7;
    }
    
    return shadowFactor;
}

// Enhanced sun shadow with better directional shadows
float calculateSunShadow(vec3 fragPos, vec3 sunPos, vec3 normal, float timeOfDay) {
    vec3 sunDir = normalize(sunPos - fragPos);
    float sunDistance = length(sunPos - fragPos);
    float angleFactor = dot(normal, sunDir);
    
    float shadowFactor = 1.0;
    
    // Create obvious directional shadows based on sun position
    float sunHeight = sunPos.y;
    
    // Very obvious floor shadows when sun is low
    if (fragPos.y < -0.7 && sunHeight < 3.0) {
        // Morning/evening long shadows
        if (timeOfDay < 8.0 || timeOfDay > 18.0) {
            shadowFactor = 0.1; // Very dark shadows
        } else if (timeOfDay < 10.0 || timeOfDay > 16.0) {
            shadowFactor = 0.2; // Strong shadows
        } else {
            shadowFactor = 0.4; // Medium shadows at midday
        }
    }
    
    // Wall shadows based on sun direction
    if (angleFactor < 0.2) {
        shadowFactor = 0.05; // Almost black for walls facing away from sun
    } else if (angleFactor < 0.5) {
        shadowFactor = 0.3;  // Strong shadow
    }
    
    // Create shadow patterns based on sun Z position (window direction)
    float sunZ = sunPos.z;
    if (sunZ > 0 && fragPos.z < 0) {
        // Sun is behind, create shadow in front
        shadowFactor *= 0.3;
    } else if (sunZ < -5 && fragPos.z > -5) {
        // Sun is in front, create shadow behind
        shadowFactor *= 0.4;
    }
    
    return shadowFactor;
}

// Enhanced lighting calculation with very obvious shadows
vec3 calculateLighting(vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo) {
    // Reduced ambient for more contrast
    vec3 ambient = vec3(0.05, 0.05, 0.08);
    vec3 result = ambient * albedo;
    
    // Point lights (chandelier) with enhanced shadows
    for(int i = 0; i < numLights; i++) {
        vec3 lightDir = normalize(lightPositions[i] - fragPos);
        float distance = length(lightPositions[i] - fragPos);
        
        // Enhanced attenuation
        float attenuation = 1.0 / (1.0 + 0.14 * distance + 0.07 * distance * distance);
        
        // Calculate obvious shadow
        float shadowFactor = calculateObjectShadow(fragPos, lightPositions[i], normal);
        
        // Diffuse
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 diffuse = diff * vec3(1.0, 0.9, 0.7) * lightIntensity;
        
        // Specular
        vec3 reflectDir = reflect(-lightDir, normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
        vec3 specular = spec * vec3(1.0, 0.9, 0.7) * lightIntensity * 0.5;
        
        // Apply strong shadow
        result += (diffuse + specular) * attenuation * albedo * shadowFactor;
    }
    
    // Enhanced sun lighting with much more obvious shadows
    if(sunIntensity > 0.0) {
        vec3 sunDir = normalize(sunPosition - fragPos);
        float sunDistance = length(sunPosition - fragPos);
        
        // Calculate obvious sun shadow
        float sunShadowFactor = calculateSunShadow(fragPos, sunPosition, normal, timeOfDay);
        
        // Controlled sun attenuation to prevent overexposure
        float sunAttenuation = 1.0 / (1.0 + 0.002 * sunDistance);
        
        // CLAMP sun intensity to prevent white-out
        float clampedSunIntensity = min(sunIntensity, 2.0);
        
        // Sun diffuse with controlled intensity
        float sunDiff = max(dot(normal, sunDir), 0.0);
        vec3 sunDiffuse = sunDiff * sunColor * clampedSunIntensity;
        
        // Reduced sun specular to prevent overexposure
        vec3 sunReflectDir = reflect(-sunDir, normal);
        float sunSpec = pow(max(dot(viewDir, sunReflectDir), 0.0), 32.0);
        vec3 sunSpecular = sunSpec * sunColor * clampedSunIntensity * 0.2; // Reduced specular
        
        // Apply obvious shadow and prevent overexposure
        vec3 sunContribution = (sunDiffuse + sunSpecular) * sunAttenuation * sunShadowFactor;
        sunContribution = min(sunContribution, vec3(1.5)); // Clamp to prevent white-out
        
        result += sunContribution * albedo;
    }
    
    // Enhanced time-based ambient that doesn't overexpose
    vec3 timeAmbient = vec3(0.02);
    if(timeOfDay >= 5.0 && timeOfDay <= 22.0) {
        if(timeOfDay < 6.0 || timeOfDay > 21.0) {
            // Dawn/dusk - warm but controlled
            timeAmbient = mix(vec3(0.01, 0.01, 0.03), vec3(0.08, 0.04, 0.02), 
                             sin((timeOfDay - 5.0) / 2.0 * 3.14159));
        } else {
            // Day ambience - controlled to prevent overexposure
            float dayFactor = smoothstep(6.0, 12.0, timeOfDay) - smoothstep(15.0, 21.0, timeOfDay);
            timeAmbient = mix(vec3(0.03, 0.03, 0.05), vec3(0.08, 0.08, 0.1), dayFactor);
        }
    } else {
        // Night ambience
        timeAmbient = vec3(0.005, 0.005, 0.02);
    }
    
    result += timeAmbient * albedo;
    
    // Clamp final result to prevent any white-out
    result = min(result, vec3(2.0));
    
    return result;
}

void main() {
    vec3 albedo = texture(texture1, fs_in.TexCoords).rgb;
    
    // Enhanced normal mapping with reduced strength to minimize artifacts
    vec3 normalMap = texture(texture2, fs_in.TexCoords).rgb * 2.0 - 1.0;
    normalMap.xy *= min(normalMapStrength, 0.8); // Limit normal map strength
    
    // Create TBN matrix for normal mapping
    vec3 T = normalize(fs_in.Tangent);
    vec3 B = normalize(fs_in.Bitangent);
    vec3 N = normalize(fs_in.Normal);
    
    // Ensure orthogonal TBN matrix to reduce artifacts
    T = normalize(T - dot(T, N) * N);
    B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    
    vec3 normal = normalize(TBN * normalMap);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    
    // Calculate enhanced lighting with obvious shadows
    vec3 color = calculateLighting(normal, fs_in.FragPos, viewDir, albedo);
    
    // Enhanced tone mapping with better exposure control
    color = color / (color + vec3(0.8)); // More aggressive tone mapping
    color = pow(color, vec3(1.0/2.2)); // Gamma correction
    
    // Final clamp to ensure no white-out
    color = clamp(color, vec3(0.0), vec3(1.0));
    
    FragColor = vec4(color, 1.0);
}