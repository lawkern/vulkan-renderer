/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#version 450

layout(location = 0) in vec3 Fragment_Normal;
layout(location = 1) in vec3 Fragment_Color;
layout(location = 2) in vec2 Fragment_Texture_Coordinate;
layout(location = 3) in vec3 Fragment_Position;

layout(location = 0) out vec4 Output_Color;

layout(binding = 1) uniform sampler2D Texture_Sampler;

void main(void)
{
   vec3 Light_Color = {1, 1, 0.5};

   float Ambient_Strength = 0.1;
   vec3 Ambient = Ambient_Strength * Light_Color;

   vec3 Normal = normalize(Fragment_Normal);
   vec3 Light_Direction = normalize(vec3(0, 0, 1));

   float Diffuse_Strength = max(dot(Normal, Light_Direction), 0.0);
   vec3 Diffuse = Diffuse_Strength * Light_Color;

   vec3 RGB = (Ambient + Diffuse) * Fragment_Color;

   Output_Color = vec4(RGB, 1.0f);
}
