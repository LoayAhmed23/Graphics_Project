#version 330 core

#define MAX_LIGHTS 16

// Light types
#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT 1
#define LIGHT_TYPE_SPOT 2

// Light structure matching C++ LightComponent
struct Light {
    int type;                // 0 = Directional, 1 = Point, 2 = Spot
    vec3 color;              // Light color
    vec3 position;           // Light position in world space (for Point and Spot)
    vec3 direction;          // Light direction (for Directional and Spot)
    vec2 coneAngles;         // Inner and outer cone angles for Spot lights (in radians)
    vec3 attenuation;        // Attenuation coefficients (constant, linear, quadratic)
};

// Material properties
struct Material {
    sampler2D albedo;
    sampler2D specular;
    sampler2D roughness;
    sampler2D ambient_occlusion;
    sampler2D emission;
    vec3 albedoTint;
    float specularStrength;
    float roughnessValue;
    vec3 emissionTint;
};

in Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 normal;
    vec3 view_position;
    vec3 world_position;
} fs_in;

out vec4 frag_color;

// Uniforms
uniform Material material;
uniform Light lights[MAX_LIGHTS];
uniform int lightCount;
uniform vec3 cameraPosition;

// Calculate lighting contribution from a single light
vec3 calculateLighting(Light light, vec3 normal, vec3 viewDir, vec3 albedo, float specular, float roughness) {
    vec3 lightDir;
    float attenuation = 1.0;
    
    // Calculate light direction and attenuation based on light type
    if (light.type == LIGHT_TYPE_DIRECTIONAL) {
        // Directional light
        lightDir = normalize(-light.direction);
        attenuation = 1.0;
    } else if (light.type == LIGHT_TYPE_POINT) {
        // Point light
        vec3 lightVec = light.position - fs_in.world_position;
        float distance = length(lightVec);
        lightDir = normalize(lightVec);
        
        // Calculate attenuation
        attenuation = 1.0 / (light.attenuation.x + 
                             light.attenuation.y * distance + 
                             light.attenuation.z * distance * distance);
    } else if (light.type == LIGHT_TYPE_SPOT) {
        // Spot light
        vec3 lightVec = light.position - fs_in.world_position;
        float distance = length(lightVec);
        lightDir = normalize(lightVec);
        
        // Calculate attenuation
        attenuation = 1.0 / (light.attenuation.x + 
                             light.attenuation.y * distance + 
                             light.attenuation.z * distance * distance);
        
        // Calculate spot cone attenuation
        float theta = dot(lightDir, normalize(-light.direction));
        float epsilon = light.coneAngles.x - light.coneAngles.y;
        float intensity = clamp((theta - light.coneAngles.y) / epsilon, 0.0, 1.0);
        attenuation *= intensity;
    }
    
    // Diffuse lighting
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * light.color * albedo;
    
    // Specular lighting (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), (1.0 - roughness) * 128.0);
    vec3 specularColor = spec * light.color * specular * material.specularStrength;
    
    return (diffuse + specularColor) * attenuation;
}

void main() {
    // Sample albedo texture (required)
    vec4 albedoSample = texture(material.albedo, fs_in.tex_coord);
    vec3 albedo = albedoSample.rgb * material.albedoTint;
    float alpha = albedoSample.a;
    
    // Use material properties for other values (textures are optional)
    // Default specular to white (1.0), roughness from material property, AO to 1.0, emission to black
    vec3 specularSample = vec3(1.0);
    float roughnessSample = material.roughnessValue;
    float ao = 1.0;
    vec3 emission = vec3(0.0);
    
    // Normalize the normal
    vec3 normal = normalize(fs_in.normal);
    
    // Calculate view direction
    vec3 viewDir = normalize(cameraPosition - fs_in.world_position);
    
    // Ambient lighting
    vec3 ambient = vec3(0.1) * albedo * ao;
    
    // Accumulate lighting from all lights
    vec3 totalLight = vec3(0.0);
    for (int i = 0; i < min(lightCount, MAX_LIGHTS); i++) {
        totalLight += calculateLighting(lights[i], normal, viewDir, albedo, length(specularSample), roughnessSample);
    }
    
    // Final color = Ambient + Accumulated Light + Emission
    vec3 finalColor = ambient + totalLight + emission;
    
    frag_color = vec4(finalColor, alpha);
}
