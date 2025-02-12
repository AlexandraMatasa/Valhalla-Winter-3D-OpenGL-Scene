#version 410 core

in vec3 fNormal;
in vec4 fPosEye;
in vec2 fTexCoords;
in vec4 FragPosLightSpace;
in vec3 fFragPosWorld;

out vec4 fColor;

// Lighting
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 lightPos;

// Textures
uniform sampler2D diffuseTexture;
uniform sampler2D specularTexture;
uniform sampler2D shadowMap;

// Point light (punctiform)
uniform vec3 pointLightPos;  
uniform vec3 pointLightColor; 
uniform float pointLightConstant;   
uniform float pointLightLinear;     
uniform float pointLightQuadratic;

// Fog uniforms
uniform vec3 fogColor;    
uniform float fogStart;   
uniform float fogEnd;

vec3 ambient;
float ambientStrength = 0.2f;
vec3 diffuse;
vec3 specular;
float specularStrength = 0.5f;
float shininess = 32.0f;

float computeShadow()
{
    vec3 normalizedCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;

    normalizedCoords = normalizedCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, normalizedCoords.xy).r;

    float currentDepth = normalizedCoords.z;

    float bias = max(0.05f * (1.0f - dot(normalize(fNormal), lightDir)), 0.005f);

    float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;

    return shadow;
}

void computeLightComponents()
{
    vec3 cameraPosEye = vec3(0.0f);

    vec3 normalEye = normalize(fNormal);
	
    // Compute light direction dynamically
    vec3 lightDirN = normalize(lightPos - fPosEye.xyz);

    vec3 viewDirN = normalize(cameraPosEye - fPosEye.xyz);

    ambient = ambientStrength * lightColor;

    diffuse = max(dot(normalEye, lightDirN), 0.0f) * lightColor;

    vec3 reflection = reflect(-lightDirN, normalEye);
    float specCoeff = pow(max(dot(viewDirN, reflection), 0.0f), shininess);
    specular = specularStrength * specCoeff * lightColor;
}

vec3 computePointLight(vec3 fragPos, vec3 normal, vec3 viewDir) 
{
    vec3 toLight = pointLightPos - fragPos;
    float dist   = length(toLight);
    vec3 Lp      = normalize(toLight);

    float attenuation = 1.0 / (pointLightConstant 
                             + pointLightLinear * dist 
                             + pointLightQuadratic * dist * dist);

    float diff = max(dot(normal, Lp), 0.0);
    vec3 diffusePL = pointLightColor * diff * attenuation;

    vec3 reflectDir = reflect(-Lp, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); // Shininess = 32
    vec3 specularPL = 0.5 * spec * pointLightColor * attenuation; // Specular strength = 0.5

    vec3 ambientPL = 0.1 * pointLightColor;

    return ambientPL + diffusePL + specularPL;
}


void main() 
{
    computeLightComponents();

    float shadow = computeShadow();

    ambient *= texture(diffuseTexture, fTexCoords).rgb;
    diffuse *= texture(diffuseTexture, fTexCoords).rgb;
    specular *= texture(specularTexture, fTexCoords).rgb;

    vec3 toLight = pointLightPos - fFragPosWorld;
    float dist = length(toLight);           
    vec3 Lp = normalize(toLight);           

    float attenuation = 1.0 / (pointLightConstant 
                             + pointLightLinear * dist 
                             + pointLightQuadratic * dist * dist);

    float diffPL = max(dot(normalize(fNormal), Lp), 0.0);
    vec3 diffusePL = pointLightColor * diffPL * attenuation;

    vec3 viewDir = normalize(-fFragPosWorld);
    vec3 reflectDir = reflect(-Lp, normalize(fNormal));
    float specPL = pow(max(dot(viewDir, reflectDir), 0.0), 32.0); // Shininess = 32
    vec3 specularPL = pointLightColor * specPL * attenuation;

    vec3 ambientPL = 0.1 * pointLightColor;

    vec3 pointLightResult = ambientPL + diffusePL + specularPL;

    vec3 color = min((ambient + (1.0f - shadow) * diffuse) + (1.0f - shadow) * specular + pointLightResult, 1.0f);
    
    float distToCam = length(fPosEye.xyz);
    float fogFactor = clamp((fogEnd - distToCam) / (fogEnd - fogStart), 0.0, 1.0); // Linear fog
    vec3 finalColor = mix(fogColor, color, fogFactor);

    fColor = vec4(finalColor, 1.0);
}
