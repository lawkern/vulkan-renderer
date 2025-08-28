/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: This file contains platform-specific code for Linux, intended to be
// shared by the different windowing system entry points (Wayland, Xlib).

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>

#include <stdarg.h>
#include <stdio.h>

static LOG(Log)
{
   va_list Arguments;

   va_start(Arguments, Format);
   vprintf(Format, Arguments);
   va_end(Arguments);

   // NOTE: Flush so that \r functions properly.
   fflush(stdout);
}

static READ_ENTIRE_FILE(Read_Entire_File)
{
   string Result = {0};

   struct stat File_Information;
   if(stat(Path, &File_Information) != 0)
   {
      Log("Failed to determine size of file %s.\n", Path);
   }
   else
   {
      int File = open(Path, O_RDONLY);
      if(File == -1)
      {
         Log("Failed to open file %s.\n", Path);
      }
      else
      {
         size Total_Size = File_Information.st_size;

         // NOTE: Add an extra byte to the allocation for a null terminator.
         u8 *Data = mmap(0, Total_Size+1, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
         if(!Data)
         {
            Log("Failed to allocate file %s.\n", Path);
         }
         else
         {
            size Length = 0;
            while(Length < Total_Size)
            {
               size Single_Read = read(File, Data+Length, Total_Size-Length);
               if(Single_Read == 0)
               {
                  break; // NOTE: Done.
               }
               else if(Single_Read == -1)
               {
                  break; // NOTE: Failed read, number of bytes read is checked below.
               }
               else
               {
                  Length += Single_Read;
               }
            }

            if(Length != Total_Size)
            {
               Log("Failed to read entire file %s, (%lu of %lu bytes read).\n", Path, Length, Total_Size);
               Free_Entire_File(Data, Total_Size);
            }
            else
            {
               // NOTE: Null terminate.
               Data[Length] = 0;

               // NOTE: Success.
               Result.Data = Data;
               Result.Length = Length;
            }
         }
      }
   }

   return(Result);
}

static FREE_ENTIRE_FILE(Free_Entire_File)
{
   // NOTE: This assumes we added an extra byte for the null terminator when
   // allocating file memory.
   if(munmap(Data, Length+1) == -1)
   {
      Log("Failed to unmap loaded file.\n");
   }
}

static inline float Compute_Seconds_Elapsed(struct timespec *Start, struct timespec *End)
{
   float Seconds_Elapsed = 1.0f / 60.0f;

   clock_gettime(CLOCK_MONOTONIC, End);
   if(Start->tv_sec || Start->tv_nsec)
   {
      time_t Full_Seconds_Elapsed = End->tv_sec - Start->tv_sec;
      time_t Nanoseconds_Elapsed  = End->tv_nsec - Start->tv_nsec;

      Seconds_Elapsed = Full_Seconds_Elapsed + (1e-9 * Nanoseconds_Elapsed);
   }
   *Start = *End;

#if DEBUG
   static u64 Count = 0;
   if((Count % 60) == 0)
   {
      Log("Elapsed Time: %fms \r", Seconds_Elapsed * 1000.0f);
   }
   Count++;
#endif

   return(Seconds_Elapsed);
}
