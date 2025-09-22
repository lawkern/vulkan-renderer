/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

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
