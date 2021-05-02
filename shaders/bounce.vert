#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
    float time;
    float power;
} constants;

// Reduced init size of vertex to allow for room to bounce
#define INIT_SIZE 0.75
// Separate power modifier
#define POWER 0.35

void main() {
    vec3 pos = inPosition;
    pos *= INIT_SIZE;
    pos *= 1.0 + (constants.power * POWER);
    
    gl_Position = vec4(pos, 1.0);
    fragColor = inColor;
}