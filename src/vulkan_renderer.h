/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct {
   vec3 Position;
   vec3 Color;
   vec2 Texture_Coordinate;
} vertex;

typedef struct {
   mat4 Model;
   mat4 View;
   mat4 Projection;
} uniform_buffer_object;

typedef struct {
   VkSemaphore Image_Available_Semaphore;
   VkFence In_Flight_Fence;

   VkDescriptorSet Descriptor_Set;
   VkCommandBuffer Command_Buffer;

   VkBuffer Uniform_Buffer;
   VkDeviceMemory Uniform_Buffer_Memory;
   void *Mapped_Uniform_Buffer;
} vulkan_frame;

typedef struct {
   VkInstance Instance;
   VkPhysicalDevice Physical_Device;
   VkSurfaceKHR Surface;

   VkDevice Device;
   VkPhysicalDeviceFeatures Physical_Device_Enabled_Features;
   VkPhysicalDeviceProperties Physical_Device_Properties;

   void *Platform_Context;
   arena Arena;
   arena Scratch;

   VkSwapchainKHR Swapchain;
   VkExtent2D Swapchain_Extent;
   VkFormat Swapchain_Image_Format;

   u32 Swapchain_Image_Count;
   VkImage *Swapchain_Images;
   VkImageView *Swapchain_Image_Views;
   VkFramebuffer *Swapchain_Framebuffers;
   VkSemaphore *Render_Finished_Semaphores;

   VkShaderModule Vertex_Shader;
   VkShaderModule Fragment_Shader;

   VkRenderPass Render_Pass;
   VkPipelineLayout Pipeline_Layout;
   VkPipeline Graphics_Pipeline;

   VkDescriptorSetLayout Descriptor_Set_Layout;
   VkDescriptorPool Descriptor_Pool;

   VkBuffer Vertex_Buffer;
   VkDeviceMemory Vertex_Buffer_Memory;

   VkBuffer Index_Buffer;
   VkDeviceMemory Index_Buffer_Memory;

   VkImage Texture_Image;
   VkDeviceMemory Texture_Image_Memory;
   VkImageView Texture_Image_View;
   VkSampler Texture_Sampler;

   VkImage Depth_Image;
   VkFormat Depth_Image_Format;
   VkDeviceMemory Depth_Image_Memory;
   VkImageView Depth_Image_View;

   VkCommandPool Command_Pool;
   vulkan_frame Frames[MAX_FRAMES_IN_FLIGHT];

   u32 Compute_Queue_Family_Index;
   u32 Graphics_Queue_Family_Index;
   u32 Present_Queue_Family_Index;

   VkQueue Compute_Queue;
   VkQueue Graphics_Queue;
   VkQueue Present_Queue;

   u32 Frame_Index;
   bool Framebuffer_Resized;
} vulkan_context;

#define VK_CHECK(Result)                                                \
   do {                                                                 \
      VkResult Err = (Result);                                          \
      if(Err != VK_SUCCESS)                                             \
      {                                                                 \
         fprintf(stderr, "Vulkan Error: %s\n", string_VkResult(Err));   \
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
