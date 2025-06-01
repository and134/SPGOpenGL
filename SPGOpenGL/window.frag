#version 330 core
in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs;

out vec4 FragColor;

uniform sampler2D windowFrame;
uniform sampler2D landscape;
uniform vec3 sunPosition;
uniform float timeOfDay;
uniform vec3 viewPos;
uniform float sunIntensity;

void main() {
    vec4 frameColor = texture(windowFrame, fs.TexCoords);
    
    if (frameColor.a < 0.05) {
        // Zona transparentã - peisaj prin fereastrã
        vec2 correctedTexCoords = fs.TexCoords;
        vec3 landscapeColor = texture(landscape, correctedTexCoords).rgb;
        
        // Calculeazã lumina naturalã bazatã pe timp ?i intensitatea soarelui
        float naturalLight = 1.0;
        if (sunIntensity <= 0.0) {
            // Dacã soarele e oprit complet, fã foarte întunecat
            naturalLight = 0.001;
        } else if (timeOfDay < 6.0 || timeOfDay > 18.0) {
            naturalLight = 0.02;
        } else if (timeOfDay >= 6.0 && timeOfDay <= 8.0) {
            naturalLight = 0.1 + (timeOfDay - 6.0) / 2.0 * 0.9;
        } else if (timeOfDay >= 16.0 && timeOfDay <= 18.0) {
            naturalLight = 1.0 - (timeOfDay - 16.0) / 2.0 * 0.8;
        }
        
        // Aplicã intensitatea manualã a soarelui
        naturalLight *= sunIntensity;
        
        // Culoarea timpului din zi
        vec3 timeColor = vec3(1.0);
        if (sunIntensity > 0.0) { // Doar dacã soarele e pornit
            if (timeOfDay >= 5.0 && timeOfDay <= 8.0) {
                float morning = clamp((timeOfDay - 5.0) / 3.0, 0.0, 1.0);
                timeColor = mix(vec3(1.0, 0.4, 0.1), vec3(1.0, 1.0, 0.9), morning);
            } else if (timeOfDay >= 17.0 && timeOfDay <= 19.0) {
                float evening = clamp((timeOfDay - 17.0) / 2.0, 0.0, 1.0);
                timeColor = mix(vec3(1.0, 1.0, 0.9), vec3(1.0, 0.2, 0.05), evening);
            } else if (timeOfDay < 6.0 || timeOfDay > 19.0) {
                timeColor = vec3(0.05, 0.1, 0.3);
            }
        } else {
            // Dacã soarele e oprit, folose?te o culoare foarte întunecatã
            timeColor = vec3(0.01, 0.01, 0.02);
        }
        
        landscapeColor *= naturalLight * timeColor;
        
        // Efecte solare doar dacã soarele e pornit
        if (sunIntensity > 0.1) {
            vec3 sunDir = normalize(sunPosition - fs.FragPos);
            float sunDot = dot(sunDir, -fs.Normal);
            
            if (sunDot > 0.0) {
                // Raze de soare
                float sunBeam = pow(max(sunDot, 0.0), 2.0) * sunIntensity;
                landscapeColor += vec3(1.0, 0.9, 0.7) * sunBeam * 1.5;
                
                // Reflexii
                vec3 viewDir = normalize(viewPos - fs.FragPos);
                vec3 reflectDir = reflect(-sunDir, fs.Normal);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
                landscapeColor += spec * vec3(1.0, 0.8, 0.5) * sunIntensity * 0.8;
            }
        }
        
        FragColor = vec4(landscapeColor, 0.95);
        
    } else {
        // Rama ferestrei sau sticla
        if (frameColor.a > 0.8) {
            // Rama
            vec3 frameColorRGB = frameColor.rgb;
            float ambient = 0.1 + sunIntensity * 0.2; // Redus ambientul pentru mai mult contrast
            frameColorRGB *= ambient;
            FragColor = vec4(frameColorRGB, 1.0);
        } else {
            // Sticla
            vec3 glassColor = vec3(0.9, 0.95, 1.0);
            float ambient = 0.2 + sunIntensity * 0.3; // Redus ambientul pentru mai mult contrast
            glassColor *= ambient;
            FragColor = vec4(glassColor, 1.0);
        }
    }
}