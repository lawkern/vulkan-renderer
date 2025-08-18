/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: Anything not specific to Vulkan should be defined here.

#define Array_Count(Array) (size)(sizeof(Array) / sizeof((Array)[0]))
#define Minimum(A, B) ((A) < (B) ? (A) : (B))
#define Maximum(A, B) ((A) > (B) ? (A) : (B))

#define Assert(Cond) do { if(!(Cond)) { __builtin_trap(); } } while(0)
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

typedef struct {
   size Length;
   u8 *Data;
} string;

#include <stdlib.h>
#include <string.h>

static inline bool Strings_Are_Equal(const char *A, const char *B)
{
   bool Result = (strcmp(A, B) == 0);
   return(Result);
}

typedef struct {
   u8 *Base;
   size Size;
   size Used;
} arena;

static inline arena Make_Arena(size Size)
{
   arena Result = {0};
   Result.Base = calloc(1, Size);
   Result.Size = Size;

   Assert(Result.Base);

   return(Result);
}

static inline void Reset_Arena(arena *Arena)
{
   Arena->Used = 0;
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
