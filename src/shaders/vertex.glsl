#version 430 core
layout (location = 0) in vec3 aPos;
layout (std430, binding = 0) buffer Particles
{
    vec4 particleData[];
};

void main()
{
    vec4 particle = particleData[gl_InstanceID];
    gl_Position = vec4(aPos.x + particle.x, aPos.y + particle.y, 0.0, 1.0);
}