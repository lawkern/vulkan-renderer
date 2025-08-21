/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#version 450

layout(location = 0) in vec3 Vertex_Position;
layout(location = 1) in vec3 Vertex_Color;

layout(location = 0) out vec3 Fragment_Color;

layout(binding = 0) uniform uniform_buffer_object {
   mat4 Model;
   mat4 View;
   mat4 Projection;
} UBO;

void main(void)
{
   Fragment_Color = Vertex_Color;
   gl_Position = UBO.Projection * UBO.View * UBO.Model * vec4(Vertex_Position, 1.0f);
}
