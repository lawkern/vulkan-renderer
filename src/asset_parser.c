/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: This is not intended to be a robust parser in the face of arbitrary
// input. We assume we are creating the .glb file ourselves, and any error
// checking is for our own sanity.

// NOTE: Basic JSON parsing API.
static string Find_Json_Array(string Json, string Key);
static int Json_Array_Count(string Array);
static string Next_Json_Object(string Json);
static int Find_Json_Integer(string Json, string Key);
static string Find_Json_String(string Json, string Key);

// NOTE: GLB file parsing.
static void Parse_GLB(gltf_scene *Result, arena *Arena, arena Scratch, char *Path)
{
   string File = Read_Entire_File(Path);
   Assert(File.Length && File.Data);

   u8 *At = File.Data;
   u8 *End = File.Data + File.Length;

   glb_header *Header = (glb_header *)At;
   if(Header->Magic != GLB_MAGIC_NUMBER)
   {
      Log("Failed to parse %s: it's not a real glb file.\n", Path);
      Invalid_Code_Path;
   }
   At += sizeof(*Header);

   glb_chunk_header *Json_Header = (glb_chunk_header *)At;
   if(Json_Header->Chunk_Type != GLB_CHUNK_TYPE_JSON)
   {
      Log("Failed to parse %s: it's missing its JSON chunk.\n", Path);
      Invalid_Code_Path;
   }
   At += sizeof(*Json_Header);

   string Json = Copy_String(&Scratch, At, Json_Header->Chunk_Length);
   At += Json.Length;

   glb_chunk_header *Binary_Header = (glb_chunk_header *)At;
   if(Binary_Header->Chunk_Type != GLB_CHUNK_TYPE_BINARY)
   {
      Log("Failed to parse %s: it's missing its binary chunk.\n", Path);
      Invalid_Code_Path;
   }
   At += sizeof(*Binary_Header);

   string Binary = Copy_String(Arena, At, Binary_Header->Chunk_Length);
   At += Binary.Length;

   // Log("%.*s\n", (int)Json.Length, Json.Data);

#  define KEY(Literal) S("\"" Literal "\"")

   // NOTE: Parse accessors.
   string Json_Accessors = Find_Json_Array(Json, KEY("accessors"));
   if(Json_Accessors.Length)
   {
      Result->Accessor_Count = Json_Array_Count(Json_Accessors);
      Result->Accessors = Allocate(Arena, gltf_accessor, Result->Accessor_Count);

      for(int Accessor_Index = 0; Accessor_Index < Result->Accessor_Count; ++Accessor_Index)
      {
         gltf_accessor *Accessor = Result->Accessors + Accessor_Index;

         Accessor->Buffer_View    = Find_Json_Integer(Json_Accessors, KEY("bufferView"));
         Accessor->Offset         = Find_Json_Integer(Json_Accessors, KEY("byteOffset"));
         Accessor->Count          = Find_Json_Integer(Json_Accessors, KEY("count"));
         Accessor->Component_Type = Find_Json_Integer(Json_Accessors, KEY("componentType"));

         string Name = Find_Json_String(Json_Accessors, KEY("type"));
         for(int Type = 0; Type < GLTF_ACCESSOR_TYPE_COUNT; ++Type)
         {
            if(Strings_Are_Equal(Name, GLTF_Accessor_Type_Names[Type]))
            {
               Accessor->Type = Type;
               break;
            }
         }

         Json_Accessors = Next_Json_Object(Json_Accessors);
      }
   }

   // NOTE: Parse buffer views.
   string Json_Buffer_Views = Find_Json_Array(Json, KEY("bufferViews"));
   if(Json_Buffer_Views.Length)
   {
      Result->Buffer_View_Count = Json_Array_Count(Json_Buffer_Views);
      Result->Buffer_Views = Allocate(Arena, gltf_buffer_view, Result->Buffer_View_Count);

      for(int Buffer_View_Index = 0; Buffer_View_Index < Result->Buffer_View_Count; ++Buffer_View_Index)
      {
         gltf_buffer_view *Buffer_View = Result->Buffer_Views + Buffer_View_Index;
         Buffer_View->Buffer = Find_Json_Integer(Json_Buffer_Views, KEY("buffer"));
         Buffer_View->Offset = Find_Json_Integer(Json_Buffer_Views, KEY("byteOffset"));
         Buffer_View->Length = Find_Json_Integer(Json_Buffer_Views, KEY("byteLength"));
         Buffer_View->Stride = Find_Json_Integer(Json_Buffer_Views, KEY("byteStride"));

         Json_Buffer_Views = Next_Json_Object(Json_Buffer_Views);
      }
   }
#  undef KEY

   Free_Entire_File(File.Data, File.Length);
}

// NOTE: The basic JSON parsing implementation. This is the bare minimum parsing
// we need to pull data from a trusted GLB file. It prioritizes simplicity, does
// not handle basic edge cases of JSON, and is not particularly fast. At some
// point we'll probably move it to a build step to pack GLB assets into our own
// binary format.

static string Find_Json_By_Key(string Json, string Key)
{
   // NOTE: Return the remainder of the JSON string following the first
   // encountered instance of the requested key (so an empty string if the key
   // isn't found). The caller is responsible for pulling the data out/trimming
   // the excess.

   cut Keys = {0};
   Keys.After = Json;

   while(Keys.After.Length)
   {
      Keys = Cut(Keys.After, ':');
      if(Has_Suffix(Keys.Before, Key))
      {
         break;
      }
   }

   string Result = Keys.After;
   return(Result);
}

static string Find_Json_Array(string Json, string Key)
{
   // NOTE: Return a string containing the contents of a JSON array with the
   // specified key. The result does not include the enclosing brackets.

   // NOTE: This function assumes the key is unique - the same key in a nested
   // object earlier in the JSON string will interfere with this lookup.

   string Result = {0};

   string Array = Find_Json_By_Key(Json, Key);
   if(Array.Length && Has_Prefix_Then_Remove(&Array, S("[")))
   {
      u8 *Pos = Array.Data;
      u8 *End = Array.Data + Array.Length;

      bool In_String = false;
      int Brackets = 1;
      while(Pos < End && Pos[0] && Brackets > 0)
      {
         if(Pos[0] == '"' && (Pos == Array.Data || Pos[-1] != '\\'))
         {
            In_String = !In_String;
         }
         else if(!In_String)
         {
            if     (Pos[0] == '[') Brackets++;
            else if(Pos[0] == ']') Brackets--;
         }
         Pos++;
      }

      if(Brackets != 0)
      {
         Log("JSON array %.*s was not bracketed properly.\n", (int)Key.Length, Key.Data);
      }
      else
      {
         Result = Span_String(Array.Data, Pos-1); // NOTE: Don't include trailing bracket.
      }
   }

   return(Result);
}

static int Json_Array_Count(string Array)
{
   // NOTE: Count the number of items in the provided JSON string. Array is
   // assumed to be a comma-delimited list of elements (no enclosing brackets),
   // or an empty string.

   int Result = 0;
   if(Array.Length)
   {
      u8 *Pos = Array.Data;
      u8 *End = Array.Data + Array.Length;

      bool In_String = false;
      int Braces = 0;
      int Brackets = 0;

      Result = 1;
      while(Pos < End && Pos[0])
      {
         if(Pos[0] == '"' && (Pos == Array.Data || Pos[-1] != '\\'))
         {
            In_String = !In_String;
         }
         else if(!In_String)
         {
            switch(Pos[0])
            {
               case '{': { Braces++;   } break;
               case '}': { Braces--;   } break;
               case '[': { Brackets++; } break;
               case ']': { Brackets--; } break;
               case ',': {
                  // NOTE: This assumes no trailing commas.
                  if(Braces == 0 && Brackets == 0)
                  {
                     Result++;
                  }
               } break;
            }
         }
         Pos++;
      }
   }

   return(Result);
}

static string Next_Json_Object(string Json)
{
   string Result = {0};
   if(Has_Prefix_Then_Remove(&Json, S("{")))
   {
      u8 *Pos = Json.Data;
      u8 *End = Json.Data + Json.Length;

      bool In_String = false;
      int Braces = 1;

      while(Pos < End && Pos[0] && Braces > 0)
      {
         if(Pos[0] == '"' && (Pos == Json.Data || Pos[-1] != '\\'))
         {
            In_String = !In_String;
         }
         else if(!In_String)
         {
            if     (Pos[0] == '{') Braces++;
            else if(Pos[0] == '}') Braces--;
         }
         Pos++;
      }

      if(Braces != 0)
      {
         Log("JSON object was not braced properly.\n");
      }
      else
      {
         if(Pos[0] == ',')
         {
            Pos++;
         }
         Result = Span_String(Pos, Json.Data+Json.Length);
      }
   }

   return(Result);
}

static int Find_Json_Integer(string Json, string Key)
{
   int Result = 0;

   string Integer = Find_Json_By_Key(Json, Key);
   if(Integer.Length)
   {
      int Sign = (Has_Prefix_Then_Remove(&Integer, S("-"))) ? -1 : 1;

      u8 *Pos = Integer.Data;
      u8 *End = Integer.Data + Integer.Length;

      int Value = 0;
      while(Pos < End && Pos[0] >= '0' && Pos[0] <= '9')
      {
         int Digit = Pos[0] - '0';
         Value = (Value * 10) + Digit;
         Pos++;
      }
      Result = Sign * Value;
   }

   return(Result);
}

static string Find_Json_String(string Json, string Key)
{
   string Result = {0};

   string String = Find_Json_By_Key(Json, Key);
   if(String.Length && Has_Prefix_Then_Remove(&String, S("\"")))
   {
      u8 *Pos = String.Data;
      u8 *End = Json.Data + Json.Length;

      bool In_String = true;
      while(Pos < End && Pos[0] && In_String)
      {
         if(Pos[0] == '"' && (Pos == Json.Data || Pos[-1] != '\\'))
         {
            In_String = false;
         }
         else
         {
            Pos++;
         }
      }

      if(In_String)
      {
         Log("JSON string was not quoted properly.\n");
      }
      else
      {
         Result = Span_String(String.Data, String.Data + (Pos-String.Data));
      }

   }

   return(Result);
}
