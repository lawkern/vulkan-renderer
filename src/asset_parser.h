/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#define GLB_MAGIC_NUMBER      0x46546C67 // glTF
#define GLB_CHUNK_TYPE_JSON   0x4E4F534A // JSON
#define GLB_CHUNK_TYPE_BINARY 0x004E4942 // BIN

typedef struct {
   u32 Magic;
   u32 Version;
   u32 Length;
} glb_header;

typedef struct {
   u32 Chunk_Length;
   u32 Chunk_Type;
} glb_chunk_header;

typedef enum {
   GLTF_ACCESSOR_COMPONENT_S8  = 5120,
   GLTF_ACCESSOR_COMPONENT_U8  = 5121,
   GLTF_ACCESSOR_COMPONENT_S16 = 5122,
   GLTF_ACCESSOR_COMPONENT_U16 = 5123,
   // Signed int not supported.
   GLTF_ACCESSOR_COMPONENT_U32 = 5125,
   GLTF_ACCESSOR_COMPONENT_F32 = 5126,
} gltf_component_type;

typedef enum {
   GLTF_ACCESSOR_TYPE_SCALAR,
   GLTF_ACCESSOR_TYPE_VEC2,
   GLTF_ACCESSOR_TYPE_VEC3,
   GLTF_ACCESSOR_TYPE_VEC4,
   GLTF_ACCESSOR_TYPE_MAT2,
   GLTF_ACCESSOR_TYPE_MAT3,
   GLTF_ACCESSOR_TYPE_MAT4,

   GLTF_ACCESSOR_TYPE_COUNT,
} gltf_accessor_type;

static string GLTF_Accessor_Type_Names[GLTF_ACCESSOR_TYPE_COUNT] =
{
   [GLTF_ACCESSOR_TYPE_SCALAR] = S("SCALAR"),
   [GLTF_ACCESSOR_TYPE_VEC2]   = S("VEC2"),
   [GLTF_ACCESSOR_TYPE_VEC3]   = S("VEC3"),
   [GLTF_ACCESSOR_TYPE_VEC4]   = S("VEC4"),
   [GLTF_ACCESSOR_TYPE_MAT2]   = S("MAT2"),
   [GLTF_ACCESSOR_TYPE_MAT3]   = S("MAT3"),
   [GLTF_ACCESSOR_TYPE_MAT4]   = S("MAT4"),
};

typedef struct {
   int Buffer_View;
   int Offset;
   int Count;
   gltf_component_type Component_Type;
   gltf_accessor_type Type;
} gltf_accessor;

typedef struct {
   int Position;
   int Normal;
   int Texcoord_0;
   int Texcoord_1;
   int Color_0;
   int Color_1;
   int Indices;
} gltf_primitive;

typedef struct {
   int Primitive_Count;
   gltf_primitive *Primitives;
} gltf_mesh;

typedef struct {
   int Buffer;
   int Offset;
   int Length;
   int Stride;
} gltf_buffer_view;

typedef struct {
   int Length;
} gltf_buffer;

typedef struct {
   int Mesh_Count;
   gltf_mesh *Meshes;

   int Accessor_Count;
   gltf_accessor *Accessors;

   int Buffer_View_Count;
   gltf_buffer_view *Buffer_Views;

   int Buffer_Count;
   gltf_buffer *Buffers;

   size Binary_Size;
   u8 *Binary_Data;
} gltf_scene;
