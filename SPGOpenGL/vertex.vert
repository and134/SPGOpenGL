#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNorm;
layout(location=2) in vec2 aTex;

uniform mat4 mvpMatrix;
uniform mat4 normalMatrix;

out VS_OUT {
    vec3  FragPos;
    vec3  Normal;
    vec2  TexCoords;
} vs;

void main(){
    vs.FragPos   = aPos;
    vs.Normal    = (normalMatrix * vec4(aNorm,0.0)).xyz;
    vs.TexCoords = aTex;
    gl_Position  = mvpMatrix * vec4(aPos,1.0);
}
