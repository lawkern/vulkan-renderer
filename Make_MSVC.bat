@ECHO off

IF NOT EXIST build mkdir build
PUSHD build

glslc -o basic.vert.spv ../code/shaders/basic.vert
glslc -o basic.frag.spv ../code/shaders/basic.frag

set INCLUDE=%VULKAN_SDK%/Include/;%INCLUDE%
set LIB=%VULKAN_SDK%/Lib/;%LIB%

cl -nologo -Fevulkan_renderer_debug.exe   ../code/main_win32.c -DDEBUG=1 -Z7 -Od vulkan-1.lib user32.lib gdi32.lib
cl -nologo -Fevulkan_renderer_release.exe ../code/main_win32.c -DDEBUG=0 -Z7 -O2 vulkan-1.lib user32.lib gdi32.lib
