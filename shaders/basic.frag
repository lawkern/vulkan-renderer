/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#version 450

layout(location = 0) in vec3 Fragment_Color;
layout(location = 1) in vec2 Fragment_Texture_Coordinate;

layout(location = 0) out vec4 Output_Color;

layout(binding = 1) uniform sampler2D Texture_Sampler;

void main(void)
{
   vec3 RGB = mix(Fragment_Color, texture(Texture_Sampler, Fragment_Texture_Coordinate).rgb, 0.75f);
   Output_Color = vec4(RGB, 1.0f);
}
