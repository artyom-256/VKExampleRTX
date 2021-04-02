#version 460

#extension GL_NV_ray_tracing : require

// Color output.
layout(location = 0) rayPayloadInNV vec3 hitValue;

void main()
{
    // Set background color.
    hitValue = vec3(0.2, 0.2, 0.2);
}
