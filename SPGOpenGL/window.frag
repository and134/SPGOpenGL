#version 330 core
in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs;

out vec4 FragColor;

uniform sampler2D windowFrame;    // Rama geamului (cu alpha)
uniform sampler2D landscape;      // Peisajul
uniform vec3 sunPosition;
uniform float timeOfDay;
uniform vec3 viewPos;
uniform float sunIntensity;

void main() {
    // Sample textura ramei
    vec4 frameColor = texture(windowFrame, fs.TexCoords);
    
    // VERIFICARE STRICT�: landscape doar dac� alpha este FOARTE mic
    if (frameColor.a < 0.05) {
        // DOAR AICI se afi?eaz� peisajul - zone COMPLET transparente
        
        vec2 correctedTexCoords = fs.TexCoords;
        vec3 landscapeColor = texture(landscape, correctedTexCoords).rgb;
        
        // Calculeaz� intensitatea luminii naturale
        float naturalLight = 1.0;
        if (timeOfDay < 6.0 || timeOfDay > 18.0) {
            naturalLight = 0.02;
        } else if (timeOfDay >= 6.0 && timeOfDay <= 8.0) {
            naturalLight = 0.1 + (timeOfDay - 6.0) / 2.0 * 0.9;
        } else if (timeOfDay >= 16.0 && timeOfDay <= 18.0) {
            naturalLight = 1.0 - (timeOfDay - 16.0) / 2.0 * 0.8;
        }
        
        // Culori �n func?ie de timpul zilei
        vec3 timeColor = vec3(1.0);
        if (timeOfDay >= 5.0 && timeOfDay <= 8.0) {
            float morning = clamp((timeOfDay - 5.0) / 3.0, 0.0, 1.0);
            timeColor = mix(vec3(1.0, 0.4, 0.1), vec3(1.0, 1.0, 0.9), morning);
        } else if (timeOfDay >= 17.0 && timeOfDay <= 19.0) {
            float evening = clamp((timeOfDay - 17.0) / 2.0, 0.0, 1.0);
            timeColor = mix(vec3(1.0, 1.0, 0.9), vec3(1.0, 0.2, 0.05), evening);
        } else if (timeOfDay < 6.0 || timeOfDay > 19.0) {
            timeColor = vec3(0.05, 0.1, 0.3);
        }
        
        landscapeColor *= naturalLight * timeColor;
        
        // Efectul de soare
        if (sunIntensity > 0.1) {
            vec3 sunDir = normalize(sunPosition - fs.FragPos);
            float sunDot = dot(sunDir, -fs.Normal);
            
            if (sunDot > 0.0) {
                float sunBeam = pow(max(sunDot, 0.0), 2.0) * sunIntensity;
                landscapeColor += vec3(1.0, 0.9, 0.7) * sunBeam * 1.5;
                
                vec3 viewDir = normalize(viewPos - fs.FragPos);
                vec3 reflectDir = reflect(-sunDir, fs.Normal);
                float spec = pow(max(dot(viewDir, reflectDir), 0.0), 8.0);
                landscapeColor += spec * vec3(1.0, 0.8, 0.5) * sunIntensity * 0.8;
            }
        }
        
        FragColor = vec4(landscapeColor, 0.95);
        
    } else {
        // TOATE CELELALTE ZONE - afi?eaz� DOAR rama sau culoare solid�
        
        if (frameColor.a > 0.8) {
            // Zon� cu ram� solid� - afi?eaz� rama
            vec3 frameColorRGB = frameColor.rgb;
            float ambient = 0.3 + sunIntensity * 0.2;
            frameColorRGB *= ambient;
            FragColor = vec4(frameColorRGB, 1.0);
        } else {
            // Zon� semi-transparent� - afi?eaz� culoare solid� (sticl� opac�)
            vec3 glassColor = vec3(0.9, 0.95, 1.0); // Culoare de sticl� alb�struie
            float ambient = 0.4 + sunIntensity * 0.3;
            glassColor *= ambient;
            FragColor = vec4(glassColor, 1.0);
        }
    }
}