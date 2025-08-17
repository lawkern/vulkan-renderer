/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: Platform API.

typedef struct {
   size Size;
   u8 *Memory;
} platform_file;

#define READ_ENTIRE_FILE(Name) platform_file Name(char *Path)
static READ_ENTIRE_FILE(Read_Entire_File);
