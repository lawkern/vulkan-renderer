/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#version 450

layout(location = 0) in vec2 Vertex_Position;
layout(location = 1) in vec3 Vertex_Color;

layout(location = 0) out vec3 Fragment_Color;

void main(void)
{
   Fragment_Color = Vertex_Color;
   gl_Position = vec4(Vertex_Position, 0.0f, 1.0f);
}
