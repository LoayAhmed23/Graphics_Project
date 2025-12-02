#version 330 core

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 tex_coord;
layout(location = 3) in vec3 normal;

out Varyings {
    vec4 color;
    vec2 tex_coord;
    vec3 normal;
    vec3 view_position;
    vec3 world_position;
} vs_out;

uniform mat4 transform;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 model_inverse_transpose;

void main(){
    // Transform position to clip space
    vec4 worldPos = model * vec4(position, 1.0);
    vs_out.world_position = worldPos.xyz;
    vs_out.view_position = (view * worldPos).xyz;
    gl_Position = projection * view * worldPos;
    
    // Pass through color and texture coordinates
    vs_out.color = color;
    vs_out.tex_coord = tex_coord;
    
    // Transform normal to world space
    vs_out.normal = normalize((model_inverse_transpose * vec4(normal, 0.0)).xyz);
}
