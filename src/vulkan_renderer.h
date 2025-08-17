/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

typedef struct {
   VkInstance Instance;
   VkPhysicalDevice Physical_Device;
   VkSurfaceKHR Surface;
   VkDevice Device;

   VkQueue Compute_Queue;
   VkQueue Graphics_Queue;
   VkQueue Present_Queue;
} vulkan_context;

// TODO: Remove Wayland-specific parameters.
#define INITIALIZE_VULKAN(Name) void Name(vulkan_context *VK, void *Platform_Context)
static INITIALIZE_VULKAN(Initialize_Vulkan);

#define RENDER_WITH_VULKAN(Name) void Name(vulkan_context *VK)
static RENDER_WITH_VULKAN(Render_With_Vulkan);

#define DESTROY_VULKAN(Name) void Name(vulkan_context *VK)
static DESTROY_VULKAN(Destroy_Vulkan);
