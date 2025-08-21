/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define MAX_FRAMES_IN_FLIGHT 2

typedef struct {
   vec3 Position;
   vec3 Color;
} vertex;

typedef struct {
   mat4 Model;
   mat4 View;
   mat4 Projection;
} uniform_buffer_object;

typedef struct {
   VkInstance Instance;
   VkPhysicalDevice Physical_Device;
   VkSurfaceKHR Surface;
   VkDevice Device;

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

   VkShaderModule Vertex_Shader;
   VkShaderModule Fragment_Shader;

   VkRenderPass Render_Pass;
   VkPipelineLayout Pipeline_Layout;
   VkPipeline Graphics_Pipeline;

   VkDescriptorSetLayout Descriptor_Set_Layout;
   VkDescriptorPool Descriptor_Pool;
   VkDescriptorSet Descriptor_Sets[MAX_FRAMES_IN_FLIGHT];

   VkBuffer Vertex_Buffer;
   VkDeviceMemory Vertex_Buffer_Memory;

   VkBuffer Index_Buffer;
   VkDeviceMemory Index_Buffer_Memory;

   VkImage Texture_Image;
   VkDeviceMemory Texture_Image_Memory;

   VkBuffer Uniform_Buffers[MAX_FRAMES_IN_FLIGHT];
   VkDeviceMemory Uniform_Buffer_Memories[MAX_FRAMES_IN_FLIGHT];
   void *Mapped_Uniform_Buffers[MAX_FRAMES_IN_FLIGHT];

   VkCommandPool Command_Pool;
   VkCommandBuffer Command_Buffers[MAX_FRAMES_IN_FLIGHT];

   VkSemaphore Image_Available_Semaphores[MAX_FRAMES_IN_FLIGHT];
   VkSemaphore Render_Finished_Semaphores[MAX_FRAMES_IN_FLIGHT];
   VkFence In_Flight_Fences[MAX_FRAMES_IN_FLIGHT];

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

#define INITIALIZE_VULKAN(Name) void Name(vulkan_context *VK, void *Platform_Context)
static INITIALIZE_VULKAN(Initialize_Vulkan);

#define RENDER_WITH_VULKAN(Name) void Name(vulkan_context *VK, float Frame_Seconds_Elapsed)
static RENDER_WITH_VULKAN(Render_With_Vulkan);

#define DESTROY_VULKAN(Name) void Name(vulkan_context *VK)
static DESTROY_VULKAN(Destroy_Vulkan);
