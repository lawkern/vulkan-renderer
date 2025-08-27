/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: Anything not specific to Vulkan should be defined here.

#define DEFAULT_RESOLUTION_WIDTH 640
#define DEFAULT_RESOLUTION_HEIGHT 480

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

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

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

typedef struct {
   size Length;
   u8 *Data;
} string;

#define S(Literal) (string){sizeof(Literal)-1, (u8 *)Literal}

static inline string Span_String(u8 *Begin, u8 *End)
{
   string Result = {0};
   Result.Data = Begin;
   if(Begin)
   {
      Result.Length = End - Begin;
   }

   return(Result);
}

static inline bool C_Strings_Are_Equal(const char *A, const char *B)
{
   bool Result = (strcmp(A, B) == 0);
   return(Result);
}

static bool Strings_Are_Equal(string A, string B)
{
   bool Result = (A.Length == B.Length) && (!A.Length || !memcmp(A.Data, B.Data, A.Length));
   return(Result);
}

static inline string Copy_String(arena *Arena, u8 *Source, size Length)
{
   string Result = {0};
   Result.Length = Length;
   Result.Data = Allocate(Arena, u8, Length+1);

   Copy_Memory(Result.Data, Source, Length);
   Result.Data[Length] = 0;

   return(Result);
}

static bool Has_Prefix(string String, string Prefix)
{
   bool Result = (String.Length >= Prefix.Length && Strings_Are_Equal(Prefix, Span_String(String.Data, String.Data + Prefix.Length)));
   return(Result);
}

static bool Has_Suffix(string String, string Suffix)
{
   bool Result = (String.Length >= Suffix.Length && Strings_Are_Equal(Suffix, Span_String(String.Data + (String.Length - Suffix.Length), String.Data + String.Length)));
   return(Result);
}

static string Remove_Prefix(string String, string Prefix)
{
   // NOTE: This assumes the existence of the prefix has already been checked.
   string Result = Span_String(String.Data + Prefix.Length, String.Data + String.Length);
   return(Result);
}

static string Remove_Suffix(string String, string Suffix)
{
   // NOTE: This assumes the existence of the suffix has already been checked.
   string Result = Span_String(String.Data, String.Data + (String.Length - Suffix.Length));
   return(Result);
}

static bool Has_Prefix_Then_Remove(string *String, string Prefix)
{
   bool Result = Has_Prefix(*String, Prefix);
   if(Result)
   {
      *String = Remove_Prefix(*String, Prefix);
   }
   return(Result);
}

static bool Has_Suffix_Then_Remove(string *String, string Suffix)
{
   bool Result = Has_Suffix(*String, Suffix);
   if(Result)
   {
      *String = Remove_Suffix(*String, Suffix);
   }
   return(Result);
}

typedef struct {
   string Before;
   string After;
   bool Found;
} cut;

static cut Cut(string String, u8 Separator)
{
   cut Result = {0};

   if(String.Length > 0)
   {
      u8 *Begin = String.Data;
      u8 *End = Begin + String.Length;

      u8 *Cut_Position = Begin;
      while(Cut_Position < End && *Cut_Position != Separator)
      {
         Cut_Position++;
      }

      Result.Found = (Cut_Position < End);
      Result.Before = Span_String(Begin, Cut_Position);
      Result.After = Span_String(Cut_Position + Result.Found, End);
   }

   return(Result);
}

#define TAU32 6.2831853f
#include <math.h>

typedef struct {
   float X;
   float Y;
} vec2;

typedef struct {
   float X;
   float Y;
   float Z;
} vec3;

typedef struct {
   float Elements[16];
} mat4;

static inline float Sine(float Turns)
{
   float Result = sinf(TAU32 * Turns);
   return(Result);
}

static inline float Cosine(float Turns)
{
   float Result = cosf(TAU32 * Turns);
   return(Result);
}

static inline float Square(float Value)
{
   float Result = Value * Value;
   return(Result);
}

static inline float Square_Root(float Value)
{
   float Result = sqrtf(Value);
   return(Result);
}

static inline vec3 Sub3(vec3 A, vec3 B)
{
   vec3 Result;
   Result.X = A.X - B.X;
   Result.Y = A.Y - B.Y;
   Result.Z = A.Z - B.Z;

   return(Result);
}

static inline vec3 Mul3(vec3 Vector, float Scalar)
{
   vec3 Result;
   Result.X = Vector.X * Scalar;
   Result.Y = Vector.Y * Scalar;
   Result.Z = Vector.Z * Scalar;

   return(Result);
}

static inline float Length3_Squared(vec3 Vector)
{
   float Result = Square(Vector.X) + Square(Vector.Y) + Square(Vector.Z);
   return(Result);
}

static inline float Length3(vec3 Vector)
{
   float Result = Square_Root(Length3_Squared(Vector));
   return(Result);
}

static inline vec3 Normalize3(vec3 Vector)
{
   vec3 Result = {0};

   float Length = Length3(Vector);
   if(Length != 0.0f)
   {
      Result.X = Vector.X / Length;
      Result.Y = Vector.Y / Length;
      Result.Z = Vector.Z / Length;
   }

   return(Result);
}

static inline float Dot3(vec3 A, vec3 B)
{
   float Result = A.X*B.X + A.Y*B.Y + A.Z*B.Z;
   return(Result);
}

static inline vec3 Cross3(vec3 A, vec3 B)
{
   vec3 Result;
   Result.X = A.Y*B.Z - A.Z*B.Y;
   Result.Y = A.Z*B.X - A.X*B.Z;
   Result.Z = A.X*B.Y - A.Y*B.X;

   return(Result);
}

static inline mat4 Identity(void)
{
   mat4 Result =
   {{
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1,
   }};

   return(Result);
}

static inline mat4 Translate(float X, float Y, float Z)
{
   mat4 Result =
   {{
      1, 0, 0, 0,
      0, 1, 0, 0,
      0, 0, 1, 0,
      X, Y, Z, 1,
   }};

   return(Result);
}

static inline mat4 Scale(float X, float Y, float Z)
{
   mat4 Result =
   {{
      X, 0, 0, 0,
      0, Y, 0, 0,
      0, 0, Z, 0,
      0, 0, 0, 1,
   }};

   return(Result);
}

static inline mat4 Rotate_X(float Turns)
{
   float C = Cosine(Turns);
   float S = Sine(Turns);
   float N = -S;

   mat4 Result =
   {{
      1, 0, 0, 0,
      0, C, S, 0,
      0, N, C, 0,
      0, 0, 0, 1,
   }};

   return(Result);
}

static inline mat4 Rotate_Y(float Turns)
{
   float C = Cosine(Turns);
   float S = Sine(Turns);
   float N = -S;

   mat4 Result =
   {{
      C, 0, N, 0,
      0, 1, 0, 0,
      S, 0, C, 0,
      0, 0, 0, 1,
   }};

   return(Result);
}

static inline mat4 Rotate_Z(float Turns)
{
   float C = Cosine(Turns);
   float S = Sine(Turns);
   float N = -S;

   mat4 Result =
   {{
      C, S, 0, 0,
      N, C, 0, 0,
      0, 0, 1, 0,
      0, 0, 0, 1,
   }};

   return(Result);
}

static inline mat4 Look_At(vec3 Eye, vec3 Target)
{
   // NOTE: We're assuming +z is always up in our definition of world space.
   vec3 World_Up = {0, 0, 1};

   vec3 F = Normalize3(Sub3(Target, Eye));   // +X: forward
   vec3 R = Normalize3(Cross3(F, World_Up)); // -Y: right
   vec3 U = Cross3(R, F);                    // +Z: up

   float TR = -Dot3(R, Eye);
   float TU = -Dot3(U, Eye);
   float TF = -Dot3(F, Eye);

   mat4 Result =
   {{
      R.X, U.X, -F.X, 0,
      R.Y, U.Y, -F.Y, 0,
      R.Z, U.Z, -F.Z, 0,
      TR,  TU,  -TF,  1,
   }};

   return(Result);
}

static inline mat4 Perspective(float Width, float Height, float Near, float Far)
{
   float Focal_Length = 3.0f;

   float A = Focal_Length / (Width / Height);
   float B = -Focal_Length;
   float C = Far / (Near - Far);
   float D = (Far * Near) / (Near - Far);
   float E = -1;

   mat4 Result =
   {{
      A, 0, 0, 0,
      0, B, 0, 0,
      0, 0, C, E,
      0, 0, D, 0,
   }};

   return(Result);
}
