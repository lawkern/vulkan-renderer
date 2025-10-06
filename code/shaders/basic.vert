/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#version 450

layout(location = 0) in vec3 Vertex_Position;
layout(location = 1) in vec3 Vertex_Normal;
layout(location = 2) in vec3 Vertex_Color;
layout(location = 3) in vec2 Vertex_Texture_Coordinate;

layout(location = 0) out vec3 Fragment_Normal;
layout(location = 1) out vec3 Fragment_Color;
layout(location = 2) out vec2 Fragment_Texture_Coordinate;
layout(location = 3) out vec3 Fragment_Position;

layout(binding = 0) uniform uniform_buffer_object {
   mat4 Model;
   mat4 View;
   mat4 Projection;
   vec3 Camera_Position;
} UBO;

void main(void)
{
   Fragment_Normal = Vertex_Normal;
   Fragment_Color = vec3(0, 0, 1); // Vertex_Color;
   Fragment_Texture_Coordinate = Vertex_Texture_Coordinate;
   Fragment_Position = Vertex_Position;
   gl_Position = UBO.Projection * UBO.View * UBO.Model * vec4(Vertex_Position, 1.0f);
}
