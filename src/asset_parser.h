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
   // GLTF_ACCESSOR_COMPONENT_S32 does not exist.
   GLTF_ACCESSOR_COMPONENT_U32 = 5125,
   GLTF_ACCESSOR_COMPONENT_F32 = 5126,

   GLTF_ACCESSOR_COMPONENT_COUNT,
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

typedef struct {
   string Name;
   int Component_Count;
} gltf_accessor_type_info;

static gltf_accessor_type_info GLTF_Accessor_Type_Infos[GLTF_ACCESSOR_TYPE_COUNT] =
{
   [GLTF_ACCESSOR_TYPE_SCALAR] = {S("SCALAR"), 1},
   [GLTF_ACCESSOR_TYPE_VEC2]   = {S("VEC2"),   2},
   [GLTF_ACCESSOR_TYPE_VEC3]   = {S("VEC3"),   3},
   [GLTF_ACCESSOR_TYPE_VEC4]   = {S("VEC4"),   4},
   [GLTF_ACCESSOR_TYPE_MAT2]   = {S("MAT2"),   4},
   [GLTF_ACCESSOR_TYPE_MAT3]   = {S("MAT3"),   9},
   [GLTF_ACCESSOR_TYPE_MAT4]   = {S("MAT4"),  16},
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

static inline size
Get_GLTF_Type_Size(gltf_accessor_type Type, gltf_component_type Component_Type)
{
   size Result = 0;

   int Component_Count = GLTF_Accessor_Type_Infos[Type].Component_Count;
   switch(Component_Type)
   {
      case GLTF_ACCESSOR_COMPONENT_S8:
      case GLTF_ACCESSOR_COMPONENT_U8: {
         Result = Component_Count;
      } break;

      case GLTF_ACCESSOR_COMPONENT_S16:
      case GLTF_ACCESSOR_COMPONENT_U16: {
         Result = Component_Count * 2;
      } break;

      case GLTF_ACCESSOR_COMPONENT_U32:
      case GLTF_ACCESSOR_COMPONENT_F32: {
         Result = Component_Count * 4;
      } break;

      default: {
         Invalid_Code_Path;
      } break;
   }

   return(Result);
}

static inline VkFormat
GLTF_To_Vulkan_Format(gltf_accessor_type Type, gltf_component_type Component_Type)
{
   VkFormat Result = VK_FORMAT_UNDEFINED;

   int Component_Count = GLTF_Accessor_Type_Infos[Type].Component_Count;
   switch(Component_Type)
   {
      case GLTF_ACCESSOR_COMPONENT_S8:
      case GLTF_ACCESSOR_COMPONENT_U8: {
         switch(Component_Count)
         {
            case 1: { Result = VK_FORMAT_R8_UINT; } break;
            case 2: { Result = VK_FORMAT_R8G8_UINT; } break;
            case 3: { Result = VK_FORMAT_R8G8B8_UINT; } break;
            case 4: { Result = VK_FORMAT_R8G8B8A8_UINT; } break;
         } break;
      } break;

      case GLTF_ACCESSOR_COMPONENT_S16:
      case GLTF_ACCESSOR_COMPONENT_U16: {
         switch(Component_Count)
         {
            case 1: { Result = VK_FORMAT_R16_UINT; } break;
            case 2: { Result = VK_FORMAT_R16G16_UINT; } break;
            case 3: { Result = VK_FORMAT_R16G16B16_UINT; } break;
            case 4: { Result = VK_FORMAT_R16G16B16A16_UINT; } break;
         } break;
      } break;

      case GLTF_ACCESSOR_COMPONENT_U32: {
         switch(Component_Count) {
            case 1: { Result = VK_FORMAT_R32_UINT; } break;
            case 2: { Result = VK_FORMAT_R32G32_UINT; } break;
            case 3: { Result = VK_FORMAT_R32G32B32_UINT; } break;
            case 4: { Result = VK_FORMAT_R32G32B32A32_UINT; } break;
         } break;
      } break;

      case GLTF_ACCESSOR_COMPONENT_F32: {
         switch(Component_Count)
         {
            case 1: { Result = VK_FORMAT_R32_SFLOAT; } break;
            case 2: { Result = VK_FORMAT_R32G32_SFLOAT; } break;
            case 3: { Result = VK_FORMAT_R32G32B32_SFLOAT; } break;
            case 4: { Result = VK_FORMAT_R32G32B32A32_SFLOAT; } break;
         } break;
      } break;

      default: {
         Invalid_Code_Path;
      } break;
   }

   return(Result);
}
