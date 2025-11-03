#version 330 core

in Varyings {
    vec4 color;
    vec2 tex_coord;
} fs_in;

out vec4 frag_color;

uniform vec4 tint;
uniform sampler2D tex;
uniform float alphaThreshold; // Added for alpha testing
void main(){
    //TODO: (Req 7) Modify the following line to compute the fragment color
    // by multiplying the tint with the vertex color and with the texture color 
    // 1. Sample the texture
    vec4 textureColor = texture(tex, fs_in.tex_coord);

    // 2. Check for alpha threshold
    if(textureColor.a < alphaThreshold) {
        discard; // Discard this fragment if its alpha is below the threshold
    }

    // 3. Compute the final color
    frag_color = textureColor * fs_in.color * tint;
   // frag_color = vec4(1.0);
}