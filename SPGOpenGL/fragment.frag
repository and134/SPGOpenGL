#version 330 core
in VS_OUT {
    vec3  FragPos;
    vec3  Normal;
    vec2  TexCoords;
} fs;
out vec4 FragColor;

uniform sampler2D texture1;    // diffuse
uniform sampler2D texture2;    // normal
uniform vec3     lightPos;
uniform vec3     viewPos;
uniform float    normalMapStrength;

void main(){
    // fetch normal map
    vec3 norm = texture(texture2, fs.TexCoords).rgb;
    norm = normalize((norm * 2.0 - 1.0) * normalMapStrength);

    vec3 color = texture(texture1, fs.TexCoords).rgb;

    // ambient
    vec3 ambient = 0.1 * color;

    // diffuse
    vec3 L = normalize(lightPos - fs.FragPos);
    float diff = max(dot(norm, L), 0.0);
    vec3 diffuse = diff * color;

    // specular
    vec3 V = normalize(viewPos - fs.FragPos);
    vec3 H = normalize(L + V);
    float spec = pow(max(dot(norm, H), 0.0), 32.0);
    vec3 specular = vec3(0.3) * spec;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}
