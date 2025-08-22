/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#version 450

layout(location = 0) in vec3 Vertex_Position;
layout(location = 1) in vec3 Vertex_Color;
layout(location = 2) in vec2 Vertex_Texture_Coordinate;

layout(location = 0) out vec3 Fragment_Color;
layout(location = 1) out vec2 Fragment_Texture_Coordinate;

layout(binding = 0) uniform uniform_buffer_object {
   mat4 Model;
   mat4 View;
   mat4 Projection;
} UBO;

void main(void)
{
   Fragment_Color = Vertex_Color;
   Fragment_Texture_Coordinate = Vertex_Texture_Coordinate;
   gl_Position = UBO.Projection * UBO.View * UBO.Model * vec4(Vertex_Position, 1.0f);
}
