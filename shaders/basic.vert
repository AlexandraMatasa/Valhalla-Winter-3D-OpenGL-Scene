#version 410 core

layout(location=0) in vec3 vPosition;
layout(location=1) in vec3 vNormal;
layout(location=2) in vec2 vTexCoords;

// Output for fragment shader
out vec3 fNormal;
out vec4 fPosEye;
out vec2 fTexCoords;
out vec4 FragPosLightSpace; 
out vec3 fFragPosWorld;


// Uniforms
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceTrMatrix;
uniform mat3 normalMatrix;

void main() 
{
    // World-space position
    vec4 worldPos = model * vec4(vPosition, 1.0); 
    fFragPosWorld = worldPos.xyz;

    // Eye-space position
    fPosEye = view * worldPos;

    // Normal in eye space
    fNormal = normalize(normalMatrix * vNormal);

    // Texture coordinates
    fTexCoords = vTexCoords;

    // Position in light space
    FragPosLightSpace = lightSpaceTrMatrix * worldPos;

    // Final vertex position in clip space
    gl_Position = projection * view * worldPos;
}
