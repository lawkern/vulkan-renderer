/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: Platform API.

#define READ_ENTIRE_FILE(Name) string Name(char *Path)
static READ_ENTIRE_FILE(Read_Entire_File);

#define GET_WINDOW_DIMENSIONS(Name) void Name(void *Platform_Context, int *Width, int *Height)
static GET_WINDOW_DIMENSIONS(Get_Window_Dimensions);
