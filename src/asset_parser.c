/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

typedef struct {
   u32 Magic;
   u32 Version;
   u32 Length;
} glb_header;

typedef struct {
   u32 Chunk_Length;
   u32 Chunk_Type;
} glb_chunk_header;

#define GLB_MAGIC_NUMBER      0x46546C67 // glTF
#define GLB_CHUNK_TYPE_JSON   0x4E4F534A // JSON
#define GLB_CHUNK_TYPE_BINARY 0x004E4942 // BIN

static void Parse_GLB(arena *Arena, arena Scratch, char *Path)
{
   string File = Read_Entire_File(Path);
   Assert(File.Length && File.Data);

   u8 *At = File.Data;
   u8 *End = File.Data + File.Length;

   glb_header *Header = (glb_header *)At;
   Assert(Header->Magic == GLB_MAGIC_NUMBER);
   At += sizeof(*Header);

   glb_chunk_header *JSON_Header = (glb_chunk_header *)At;
   Assert(JSON_Header->Chunk_Type == GLB_CHUNK_TYPE_JSON);
   At += sizeof(*JSON_Header);

   size JSON_Length = JSON_Header->Chunk_Length;
   char *JSON_Data = Allocate(&Scratch, char, JSON_Length+1);
   Copy_Memory(JSON_Data, At, JSON_Length);
   JSON_Data[JSON_Length] = 0;
   At += JSON_Length;

   if(At < End)
   {
      glb_chunk_header *Binary_Header = (glb_chunk_header *)At;
      Assert(Binary_Header->Chunk_Type == GLB_CHUNK_TYPE_BINARY);
      At += sizeof(*Binary_Header);

      size Binary_Length = Binary_Header->Chunk_Length;
      u8 *Binary_Data = Allocate(&Scratch, u8, Binary_Length);
      Copy_Memory(Binary_Data, At, Binary_Length);
      At += Binary_Length;
   }
   Assert(At == End);

   Log(JSON_Data);
}
