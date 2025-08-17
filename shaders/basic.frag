/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#version 450

layout(location = 0) in vec4 Fragment_Color;
layout(location = 0) out vec4 Output_Color;

void main(void)
{
   Output_Color = Fragment_Color;
}
