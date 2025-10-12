/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: API to be implemented by each platform for use by the renderer:

#define LOG(Name) void Name(char *Format, ...)
static LOG(Log);

#define READ_ENTIRE_FILE(Name) string Name(char *Path)
static READ_ENTIRE_FILE(Read_Entire_File);

#define FREE_ENTIRE_FILE(Name) void Name(u8 *Data, idx Length)
static FREE_ENTIRE_FILE(Free_Entire_File);

#define GET_WINDOW_DIMENSIONS(Name) void Name(void *Platform_Context, int *Width, int *Height)
static GET_WINDOW_DIMENSIONS(Get_Window_Dimensions);
