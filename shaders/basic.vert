/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#version 450

layout(location = 0) out vec4 Fragment_Color;

vec4 Positions[] =
{
   { 0.0f, -0.5f, 0.0f, 1.0f},
   { 0.5f,  0.5f, 0.0f, 1.0f},
   {-0.5f,  0.5f, 0.0f, 1.0f},
};

vec4 Colors[] =
{
   {1.0f, 1.0f, 0.0f, 1.0f},
   {0.0f, 1.0f, 1.0f, 1.0f},
   {1.0f, 0.0f, 1.0f, 1.0f},
};

void main(void)
{
   Fragment_Color = Colors[gl_VertexIndex];
   gl_Position = Positions[gl_VertexIndex];
}
