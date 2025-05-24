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
    // Sample texturi
    vec4 frameColor = texture(windowFrame, fs.TexCoords);
    vec3 landscapeColor = texture(landscape, fs.TexCoords).rgb;
    
    // Calculeazã intensitatea luminii naturale
    float naturalLight = 1.0;
    if (timeOfDay < 6.0 || timeOfDay > 18.0) {
        naturalLight = 0.05; // Noapte foarte întunecatã
    } else if (timeOfDay >= 6.0 && timeOfDay <= 8.0) {
        naturalLight = 0.2 + (timeOfDay - 6.0) / 2.0 * 0.8; // Rãsãrit
    } else if (timeOfDay >= 16.0 && timeOfDay <= 18.0) {
        naturalLight = 1.0 - (timeOfDay - 16.0) / 2.0 * 0.8; // Apus
    }
    
    // Culori în func?ie de timpul zilei
    vec3 timeColor = vec3(1.0);
    if (timeOfDay >= 5.0 && timeOfDay <= 8.0) {
        // Diminea?a - portocaliu blând
        float morning = clamp((timeOfDay - 5.0) / 3.0, 0.0, 1.0);
        timeColor = mix(vec3(1.0, 0.5, 0.2), vec3(1.0, 1.0, 0.95), morning);
    } else if (timeOfDay >= 17.0 && timeOfDay <= 19.0) {
        // Seara - portocaliu-ro?u
        float evening = clamp((timeOfDay - 17.0) / 2.0, 0.0, 1.0);
        timeColor = mix(vec3(1.0, 1.0, 0.95), vec3(1.0, 0.3, 0.1), evening);
    } else if (timeOfDay < 6.0 || timeOfDay > 19.0) {
        // Noapte - albastru închis
        timeColor = vec3(0.1, 0.2, 0.4);
    }
    
    // Aplicã efectele pe peisaj
    landscapeColor *= naturalLight * timeColor;
    
    // Efectul de soare direct pe sticlã
    if (sunIntensity > 0.2) {
        vec3 sunDir = normalize(sunPosition - fs.FragPos);
        vec3 viewDir = normalize(viewPos - fs.FragPos);
        vec3 reflectDir = reflect(-sunDir, fs.Normal);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
        landscapeColor += spec * vec3(1.0, 0.8, 0.6) * sunIntensity * 0.5;
    }
    
    // Blending correct între ramã ?i peisaj
    // frameColor.a = 0 ? transparent (se vede peisajul)
    // frameColor.a = 1 ? opac (se vede rama)
    vec3 finalColor = mix(landscapeColor, frameColor.rgb, frameColor.a);
    
    // Pentru zonele de sticlã (frameColor.a ~ 0), pãstreazã pu?inã transparen?ã
    float finalAlpha = max(frameColor.a, 0.95);
    
    FragColor = vec4(finalColor, finalAlpha);
}