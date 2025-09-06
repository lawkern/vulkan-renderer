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
} uniform_buffer_object;

typedef struct {
   VkBuffer Buffer;
   VkDeviceMemory Device_Memory;
   void *Mapped_Memory_Address;

   size Size;
   VkIndexType Index_Type;
} vulkan_buffer;

typedef struct {
   VkSemaphore Image_Available_Semaphore;
   VkFence In_Flight_Fence;

   VkDescriptorSet Descriptor_Set;
   VkCommandBuffer Command_Buffer;

   vulkan_buffer Uniform;
} vulkan_frame;

typedef struct {
   VkShaderModule Vertex_Shader;
   VkShaderModule Fragment_Shader;

   VkPipelineLayout Layout;
   VkPipeline Pipeline;
} vulkan_pipeline;

typedef struct {
   VkInstance Instance;
   VkPhysicalDevice Physical_Device;
   VkSurfaceKHR Surface;

   VkDevice Device;
   VkPhysicalDeviceFeatures Enabled_Physical_Device_Features;
   VkPhysicalDeviceProperties Physical_Device_Properties;

   void *Platform_Context;
   arena Arena;
   arena Scratch;

   gltf_scene Debug_Scene;

   VkSwapchainKHR Swapchain;
   VkExtent2D Swapchain_Extent;
   VkFormat Swapchain_Image_Format;

   u32 Swapchain_Image_Count;
   VkImage *Swapchain_Images;
   VkImageView *Swapchain_Image_Views;
   VkFramebuffer *Swapchain_Framebuffers;
   VkSemaphore *Render_Finished_Semaphores;

   VkSampleCountFlagBits Multisample_Count;

   VkRenderPass Basic_Render_Pass;
   vulkan_pipeline Basic_Graphics_Pipeline;

   VkDescriptorSetLayout Descriptor_Set_Layout;
   VkDescriptorPool Descriptor_Pool;

   vulkan_buffer Vertex_Positions;
   vulkan_buffer Vertex_Normals;
   vulkan_buffer Vertex_Colors;
   vulkan_buffer Vertex_Texcoords;
   vulkan_buffer Vertex_Indices;

   VkImage Texture_Image;
   VkDeviceMemory Texture_Image_Memory;
   VkImageView Texture_Image_View;
   VkSampler Texture_Sampler;

   VkImage Depth_Image;
   VkFormat Depth_Image_Format;
   VkDeviceMemory Depth_Image_Memory;
   VkImageView Depth_Image_View;

   VkImage Color_Image;
   VkDeviceMemory Color_Image_Memory;
   VkImageView Color_Image_View;

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
