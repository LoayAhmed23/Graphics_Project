#version 330

// The texture holding the scene pixels
uniform sampler2D tex;

// Read "assets/shaders/fullscreen.vert" to know what "tex_coord" holds;
in vec2 tex_coord;
out vec4 frag_color;

// Speed lines / Radial blur effect
// Creates the feeling of high speed motion with lines radiating from center

// Radial blur samples
#define BLUR_SAMPLES 16
#define BLUR_STRENGTH 0.04

// Speed lines parameters
#define LINE_COUNT 60.0
#define LINE_SPEED 2.0
#define LINE_INTENSITY 0.15

// Simple pseudo-random function
float rand(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main(){
    vec2 center = vec2(0.5, 0.5);
    vec2 toCenter = tex_coord - center;
    float dist = length(toCenter);
    
    // === RADIAL BLUR ===
    // Sample along the direction from center outward
    vec4 blurColor = vec4(0.0);
    for(int i = 0; i < BLUR_SAMPLES; i++){
        float t = float(i) / float(BLUR_SAMPLES);
        vec2 offset = toCenter * BLUR_STRENGTH * t;
        blurColor += texture(tex, tex_coord - offset);
    }
    blurColor /= float(BLUR_SAMPLES);
    
    // === SPEED LINES ===
    // Calculate angle from center
    float angle = atan(toCenter.y, toCenter.x);
    
    // Create radial lines pattern
    float lines = sin(angle * LINE_COUNT) * 0.5 + 0.5;
    
    // Make lines fade in from edges (stronger at edges)
    float edgeFade = smoothstep(0.1, 0.5, dist);
    
    // Add some variation to the lines
    float lineNoise = rand(vec2(angle * 10.0, dist * 5.0));
    lines = lines * lineNoise;
    
    // Speed lines are white/bright streaks
    vec3 speedLineColor = vec3(1.0, 1.0, 0.95);
    float lineStrength = lines * edgeFade * LINE_INTENSITY;
    
    // === COMBINE EFFECTS ===
    vec3 finalColor = blurColor.rgb;
    
    // Add speed lines on top
    finalColor = mix(finalColor, speedLineColor, lineStrength);
    
    // Add slight vignette darkening at very edges
    float vignette = 1.0 - smoothstep(0.4, 0.8, dist);
    finalColor *= mix(0.7, 1.0, vignette);
    
    // Boost overall brightness slightly for "speed" feeling
    finalColor *= 1.1;
    
    frag_color = vec4(finalColor, 1.0);
}

