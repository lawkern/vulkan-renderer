/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: Anything not specific to Vulkan should be defined here.

#define DEFAULT_RESOLUTION_WIDTH 640
#define DEFAULT_RESOLUTION_HEIGHT 480

#define Array_Count(Array) (size)(sizeof(Array) / sizeof((Array)[0]))
#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#if _MSC_VER
#  define Assert(Cond) do { if(!(Cond)) { __debugbreak(); } } while(0)
#else
#  define Assert(Cond) do { if(!(Cond)) { __builtin_trap(); } } while(0)
#endif

#define Invalid_Code_Path Assert(0)

#define Kilobytes(N) (1024 * (N))
#define Megabytes(N) (1024 * Kilobytes(N))
#define Gigabytes(N) (1024 * Megabytes(N))

#include <stdint.h>
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#include <stddef.h>
typedef ptrdiff_t size;

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "basic_string.h"
#include "basic_math.h"
#include "debug_data.c"

static inline void Copy_Memory(void *Destination, void *Source, size Size)
{
   memcpy(Destination, Source, Size);
}

#define Zero_Struct(Struct) Zero_Memory((Struct), sizeof(*(Struct)))
static inline void Zero_Memory(void *Destination, size Size)
{
   memset(Destination, 0, Size);
}

typedef struct {
   u8 *Base;
   size Size;
   size Used;
} arena;

static inline void Make_Arena(arena *Arena, size Size)
{
   Arena->Base = calloc(1, Size);
   Arena->Size = Size;
   Arena->Used = 0;

   Assert(Arena->Base);
}

static inline void Reset_Arena(arena *Arena)
{
   Arena->Used = 0;
}

static inline void Make_Arena_Once(arena *Arena, size Size)
{
   // NOTE: Assumes new arenas were previously zero-initialized.
   if(Arena->Size == 0)
   {
      Make_Arena(Arena, Size);
   }
   else
   {
      Reset_Arena(Arena);
   }
}

// TODO: Alignment!
#define Allocate(Arena, type, Count) (type *)Allocate_Size((Arena), sizeof(type)*(Count))
static inline void *Allocate_Size(arena *Arena, size Size)
{
   Assert(Arena->Used <= (Arena->Size - Size));

   void *Result = Arena->Base + Arena->Used;
   Arena->Used += Size;

   return memset(Result, 0, Size);
}
