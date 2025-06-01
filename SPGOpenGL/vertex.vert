#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNorm;
layout(location=2) in vec2 aTex;

uniform mat4 mvpMatrix;
uniform mat4 modelMatrix;
uniform mat4 normalMatrix;

out VS_OUT {
    vec3  FragPos;
    vec3  Normal;
    vec3  Tangent;
    vec3  Bitangent;
    vec2  TexCoords;
} vs;

void main(){
    vs.FragPos = (modelMatrix * vec4(aPos, 1.0)).xyz;
    vs.Normal = normalize((normalMatrix * vec4(aNorm, 0.0)).xyz);

    vec3 c1 = cross(vs.Normal, vec3(0.0, 0.0, 1.0)); 
    vec3 c2 = cross(vs.Normal, vec3(0.0, 1.0, 0.0)); 
    
    if(length(c1) > length(c2)) {
        vs.Tangent = c1;
    } else {
        vs.Tangent = c2;
    }
    vs.Tangent = normalize(vs.Tangent);
    vs.Bitangent = normalize(cross(vs.Normal, vs.Tangent));
    
    vs.TexCoords = aTex;
    gl_Position = mvpMatrix * vec4(aPos, 1.0);
}