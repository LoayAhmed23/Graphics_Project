#version 330 core

// Vertex attributes
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;

// Outputs to fragment shader (varyings)
out Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 normal;          // Normal in world space
    vec3 view;            // Vector from fragment to camera
    vec3 world_position;  // Fragment position in world space
} vs_out;

// Uniforms
uniform mat4 model;                 // M - Model matrix (object to world space)
uniform mat4 view;                  // V - View matrix (world to camera space)
uniform mat4 projection;            // P - Projection matrix
uniform mat4 model_inverse_transpose; // M_IT - Inverse-Transpose of Model matrix (for normals)
uniform vec3 cameraPosition;        // Camera/eye position in world space

void main() {
    // Transform position to world space
    vec3 world_pos = (model * vec4(position, 1.0)).xyz;
    
    // Calculate final clip space position
    gl_Position = projection * view * vec4(world_pos, 1.0);

    // Pass through color and texture coordinates
    vs_out.color = color;
    vs_out.tex_coord = tex_coord;

    // Transform normal using M_IT (handles non-uniform scaling correctly)
    // We use the xyz of the result and normalize it
    vs_out.normal = normalize((model_inverse_transpose * vec4(normal, 0.0)).xyz);

    // View vector: from fragment position to camera (will be normalized in fragment shader)
    vs_out.view = cameraPosition - world_pos;

    // World position for point/spot light distance calculations
    vs_out.world_position = world_pos;
}
