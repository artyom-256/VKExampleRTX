#version 460

#extension GL_NV_ray_tracing : require

// Shader input that contains barycentric coordinates of a hit position.
hitAttributeNV vec3 attribs;

// Color output.
layout(location = 0) rayPayloadInNV vec3 hitValue;

void main()
{
    // Transform coordinates into a gradient color.
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    hitValue = barycentricCoords;
}
