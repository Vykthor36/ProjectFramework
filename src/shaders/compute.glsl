#version 430 core

layout (local_size_x = 256) in;

struct Particle
{
    float x;
    float y;
    float xSpeed;
    float ySpeed;
};

layout(std430, binding = 0) buffer Particles
{\
    Particle particles[];
};

uniform float dTime;
void main()
{
    uint id = gl_GlobalInvocationID.x;
    particles[id].x += particles[id].xSpeed * dTime;
    particles[id].y += particles[id].ySpeed * dTime;
}