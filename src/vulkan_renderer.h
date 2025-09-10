/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include "asset_parser.h"

#define MAX_FRAMES_IN_FLIGHT 2

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
#  define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
#  define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#  define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#else
#  error Support for this platform has not been implemented yet.
#endif

typedef struct {
   mat4 Model;
   mat4 View;
   mat4 Projection;
} basic_uniform;

typedef struct {
   VkBuffer Buffer;
   VkDeviceMemory Device_Memory;
   void *Mapped_Memory_Address;

   size Size;
   VkIndexType Index_Type;
} vulkan_buffer;

typedef struct {
   VkImage Image;
   VkDeviceMemory Device_Memory;
   VkImageView View;
   VkFormat Format;
} vulkan_image;

typedef struct {
   VkSemaphore Image_Available_Semaphore;
   VkFence In_Flight_Fence;

   VkDescriptorSet Descriptor_Set;
   VkCommandBuffer Command_Buffer;

   vulkan_buffer Uniform;
} vulkan_frame;

typedef struct {
   VkPipeline Pipeline;
   VkPipelineLayout Layout;

   VkShaderModule Vertex_Shader;
   VkShaderModule Fragment_Shader;
} vulkan_pipeline;

typedef struct {
   VkSwapchainKHR Handle;
   VkExtent2D Extent;
   VkFormat Image_Format;

   vulkan_image Depth_Image;
   vulkan_image Color_Image;

   u32 Image_Count;
   u32 Max_Image_Count;
   VkImage *Images;
   VkImageView *Image_Views;
   VkFramebuffer *Framebuffers;
   VkSemaphore *Render_Finished_Semaphores;
} vulkan_swapchain;

typedef struct {
   VkPhysicalDevice Handle;
   VkPhysicalDeviceFeatures Enabled_Features;
   VkPhysicalDeviceProperties Properties;
} vulkan_physical_device;

typedef struct {
   VkInstance Instance;
   vulkan_physical_device Physical_Device;
   VkSurfaceKHR Surface;
   VkDevice Device;

   void *Platform_Context;
   arena Permanent;
   arena Scratch;

   gltf_scene Debug_Scene;

   vulkan_swapchain Swapchain;

   VkSampleCountFlagBits Multisample_Count;

   VkRenderPass Basic_Render_Pass;
   vulkan_pipeline Basic_Graphics_Pipeline;
   // vulkan_pipeline Basic_Text_Pipeline;

   VkDescriptorSetLayout Descriptor_Set_Layout;
   VkDescriptorPool Descriptor_Pool;

   vulkan_buffer Vertex_Positions;
   vulkan_buffer Vertex_Normals;
   vulkan_buffer Vertex_Colors;
   vulkan_buffer Vertex_Texcoords;
   vulkan_buffer Vertex_Indices;

   VkSampler Texture_Sampler;
   vulkan_image Debug_Texture;
   vulkan_image Debug_Text;

   VkCommandPool Command_Pool;
   vulkan_frame Frames[MAX_FRAMES_IN_FLIGHT];

   u32 Compute_Queue_Family_Index;
   u32 Graphics_Queue_Family_Index;
   u32 Present_Queue_Family_Index;

   VkQueue Compute_Queue;
   VkQueue Graphics_Queue;
   VkQueue Present_Queue;

   u32 Frame_Index;
   bool Resize_Requested;
} vulkan_context;

#define VC(Result)                                                      \
   do {                                                                 \
      VkResult Err = (Result);                                          \
      if(Err != VK_SUCCESS)                                             \
      {                                                                 \
         Log("Vulkan result check failed: %s\n", string_VkResult(Err)); \
         Invalid_Code_Path;                                             \
      }                                                                 \
   } while(0)

// NOTE: Renderer API for use by each platform:

#define INITIALIZE_VULKAN(Name) bool Name(vulkan_context *VK, void *Platform_Context)
static INITIALIZE_VULKAN(Initialize_Vulkan);

#define RENDER_WITH_VULKAN(Name) void Name(vulkan_context *VK, float Frame_Seconds_Elapsed)
static RENDER_WITH_VULKAN(Render_With_Vulkan);

#define DESTROY_VULKAN(Name) void Name(vulkan_context *VK)
static DESTROY_VULKAN(Destroy_Vulkan);
