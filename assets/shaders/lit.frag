#version 330 core

// Light type constants
#define DIRECTIONAL 0
#define POINT       1
#define SPOT        2
#define MAX_LIGHTS  16

// Light structure
struct Light {
    int type;               // DIRECTIONAL, POINT, or SPOT
    vec3 color;             // Light color/intensity
    vec3 position;          // Position in world space (for point/spot)
    vec3 direction;         // Direction FROM light source (for directional/spot)
    vec3 attenuation;       // (constant, linear, quadratic) coefficients
    vec2 coneAngles;        // (inner, outer) cone angles in radians (for spot)
};

// Material structure for Phong lighting
struct Material {
    vec3 ambient;           // Ka - Ambient reflectivity
    vec3 diffuse;           // Kd - Diffuse reflectivity  
    vec3 specular;          // Ks - Specular reflectivity
    float shininess;        // Shininess exponent (higher = sharper highlight)
};

// Inputs from vertex shader
in Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 normal;
    vec3 view;
    vec3 world_position;
} fs_in;

// Output
out vec4 frag_color;

// Uniforms
uniform Light lights[MAX_LIGHTS];
uniform int lightCount;
uniform vec3 ambientLight;          // Global ambient light intensity (Ia)
uniform Material material;

// Texture maps
uniform sampler2D tex;                      // Albedo/diffuse texture (unit 0)
uniform sampler2D specular_map;             // Specular intensity map (unit 1)
uniform sampler2D roughness_map;            // Roughness map (unit 2)
uniform sampler2D ambient_occlusion_map;    // Ambient occlusion map (unit 3)
uniform sampler2D emissive_map;             // Emission map (unit 4)

// Texture usage flags (to check if texture is bound)
uniform bool use_specular_map;
uniform bool use_roughness_map;
uniform bool use_ao_map;
uniform bool use_emissive_map;

// Other uniforms
uniform vec4 tint;                  // Color tint
uniform float alphaThreshold;       // Alpha cutoff threshold
uniform vec3 emissive_color;        // Emission color multiplier

void main() {
    // STEP 1: Normalize interpolated varyings (they may become un-normalized after interpolation)
    vec3 N = normalize(fs_in.normal);    // Surface normal
    vec3 V = normalize(fs_in.view);      // View direction (toward camera)

    // STEP 2: Sample the albedo texture
    vec4 texColor = texture(tex, fs_in.tex_coord);
    
    // Alpha test - discard fragments below threshold
    if (texColor.a < alphaThreshold) {
        discard;
    }

    // Base color from texture, vertex color and tint
    vec3 baseColor = texColor.rgb * fs_in.color.rgb * tint.rgb;

    // STEP 3: Sample additional texture maps
    
    // Specular intensity from map or material property
    vec3 specularIntensity = material.specular;
    if (use_specular_map) {
        specularIntensity = texture(specular_map, fs_in.tex_coord).rgb;
    }
    
    // Shininess from roughness map or material property
    // Roughness is inverse of shininess: high roughness = low shininess
    float shininess = material.shininess;
    if (use_roughness_map) {
        float roughness = texture(roughness_map, fs_in.tex_coord).r;
        // Convert roughness to shininess: shininess = 2 / roughness^4 - 2
        // Clamp roughness to avoid division issues
        roughness = clamp(roughness, 0.05, 1.0);
        shininess = 2.0 / pow(roughness, 4.0) - 2.0;
        shininess = clamp(shininess, 1.0, 256.0);
    }
    
    // Ambient occlusion
    float ao = 1.0;
    if (use_ao_map) {
        ao = texture(ambient_occlusion_map, fs_in.tex_coord).r;
    }
    
    // Emission
    vec3 emission = vec3(0.0);
    if (use_emissive_map) {
        emission = texture(emissive_map, fs_in.tex_coord).rgb * emissive_color;
    } else {
        emission = emissive_color;
    }

    // STEP 4: Compute ambient component (light-independent)
    // Ambient = Ka * Ia * baseColor * AO
    vec3 ambient = ambientLight * material.ambient * baseColor * ao;
    
    // Start accumulating light contributions with ambient and emission
    vec3 color = ambient + emission;

    // STEP 5: Loop through all lights
    for (int i = 0; i < lightCount && i < MAX_LIGHTS; i++) {
        Light light = lights[i];
        vec3 L;                     // Light direction (toward light)
        float attenuation = 1.0;    // Light intensity falloff

        // STEP 6: Calculate light direction based on type
        if (light.type == DIRECTIONAL) {
            // Directional light: Light direction is constant
            // We negate because 'direction' is FROM light source
            L = normalize(-light.direction);
        }
        else {
            // Point/Spot light: Calculate vector from fragment to light
            vec3 fragToLight = light.position - fs_in.world_position;
            float distance = length(fragToLight);
            L = fragToLight / distance;  // Normalize

            // STEP 7: Distance attenuation
            // Attenuation = 1 / (constant + linear*d + quadratic*d²)
            float attFactor = light.attenuation.x + 
                              light.attenuation.y * distance + 
                              light.attenuation.z * distance * distance;
            attenuation = 1.0 / max(attFactor, 0.0001);

            // STEP 8: Spot light cone attenuation
            if (light.type == SPOT) {
                // Angle between light direction and vector from light to fragment
                float theta = acos(dot(normalize(light.direction), -L));
                
                // Smooth falloff between inner and outer cone angles
                // smoothstep returns 0 when theta >= outer, 1 when theta <= inner
                attenuation *= smoothstep(light.coneAngles.y, light.coneAngles.x, theta);
            }
        }

        // STEP 9: Diffuse component (Lambert's Law)
        // Diffuse = Kd * Id * max(0, N·L) * baseColor
        float lambert = max(0.0, dot(N, L));
        vec3 diffuse = light.color * material.diffuse * lambert * baseColor;

        // STEP 10: Specular component (Phong reflection model)
        // R = reflect(-L, N) - reflection of light around normal
        // Specular = Ks * Is * max(0, R·V)^shininess
        vec3 R = reflect(-L, N);
        float spec = pow(max(0.0, dot(R, V)), shininess);
        vec3 specular = light.color * specularIntensity * spec;

        // STEP 11: Add light contribution with attenuation
        color += (diffuse + specular) * attenuation;
    }

    // Final color with alpha from texture and tint
    frag_color = vec4(color, texColor.a * tint.a);
}
