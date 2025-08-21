/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

typedef struct {
   vec3 Position;
   vec3 Color;
} vertex;

static vertex Vertices[] =
{
   {{ 1.0f,  1.0f,  1.0f}, {1.0f, 1.0f, 0}},
   {{ 1.0f, -1.0f,  1.0f}, {0, 1.0f, 1.0f}},
   {{-1.0f, -1.0f,  1.0f}, {1.0f, 0, 1.0f}},
   {{-1.0f,  1.0f,  1.0f}, {1.0f, 1.0f, 0}},

   {{ 1.0f,  1.0f, -1.0f}, {1.0f, 1.0f, 0}},
   {{ 1.0f, -1.0f, -1.0f}, {0, 1.0f, 1.0f}},
   {{-1.0f, -1.0f, -1.0f}, {1.0f, 0, 1.0f}},
   {{-1.0f,  1.0f, -1.0f}, {1.0f, 1.0f, 0}},
};

static u16 Indices[] =
{
   0, 1, 3, 1, 2, 3,
   3, 2, 6, 3, 6, 7,
   2, 1, 5, 2, 5, 6,
   1, 0, 4, 1, 4, 5,
   0, 7, 4, 0, 3, 7,
   4, 6, 5, 4, 7, 6,
};

typedef struct {
   mat4 Model;
   mat4 View;
   mat4 Projection;
} uniform_buffer_object;

static bool Vulkan_Extensions_Supported(VkExtensionProperties *Extensions, u32 Extension_Count, const char **Required_Names, u32 Required_Count)
{
   bool Result = true;
   for(u32 Required_Index = 0; Required_Index < Required_Count; ++Required_Index)
   {
      const char *Required_Name = Required_Names[Required_Index];
      bool Found = false;
      for(u32 Available_Index = 0; Available_Index < Extension_Count; ++Available_Index)
      {
         if(Strings_Are_Equal(Required_Name, Extensions[Available_Index].extensionName))
         {
            Found = true;
            break;
         }
      }
      if(!Found)
      {
         fprintf(stderr, "Error: Requested instance extension %s not available.\n", Required_Name);
         Result = false;
      }
   }
   return(Result);
}

static void Confirm_Vulkan_Instance_Extensions(const char **Required_Names, u32 Required_Count, arena Scratch)
{
   u32 Total_Count = 0;
   vkEnumerateInstanceExtensionProperties(0, &Total_Count, 0);

   VkExtensionProperties *Extensions = Allocate(&Scratch, VkExtensionProperties, Total_Count);
   vkEnumerateInstanceExtensionProperties(0, &Total_Count, Extensions);

   bool Found_All = Vulkan_Extensions_Supported(Extensions, Total_Count, Required_Names, Required_Count);
   Assert(Found_All);
}

static void Confirm_Vulkan_Device_Extensions(VkPhysicalDevice Physical_Device, const char **Required_Names, u32 Required_Count, arena Scratch)
{
   u32 Total_Count = 0;
   vkEnumerateDeviceExtensionProperties(Physical_Device, 0, &Total_Count, 0);

   VkExtensionProperties *Extensions = Allocate(&Scratch, VkExtensionProperties, Total_Count);
   vkEnumerateDeviceExtensionProperties(Physical_Device, 0, &Total_Count, Extensions);

   bool Found_All = Vulkan_Extensions_Supported(Extensions, Total_Count, Required_Names, Required_Count);
   Assert(Found_All);
}

static void Destroy_Vulkan_Swapchain(vulkan_context *VK)
{
   for(u32 Image_Index = 0; Image_Index < VK->Swapchain_Image_Count; ++Image_Index)
   {
      vkDestroyFramebuffer(VK->Device, VK->Swapchain_Framebuffers[Image_Index], 0);
      vkDestroyImageView(VK->Device, VK->Swapchain_Image_Views[Image_Index], 0);
   }
   vkDestroySwapchainKHR(VK->Device, VK->Swapchain, 0);
}

static void Create_Vulkan_Swapchain(vulkan_context *VK)
{
   VkSurfaceCapabilitiesKHR Surface_Capabilities;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VK->Physical_Device, VK->Surface, &Surface_Capabilities);

   u32 Surface_Format_Count;
   vkGetPhysicalDeviceSurfaceFormatsKHR(VK->Physical_Device, VK->Surface, &Surface_Format_Count, 0);
   Assert(Surface_Format_Count > 0);

   VkSurfaceFormatKHR *Surface_Formats = Allocate(&VK->Scratch, VkSurfaceFormatKHR, Surface_Format_Count);
   vkGetPhysicalDeviceSurfaceFormatsKHR(VK->Physical_Device, VK->Surface, &Surface_Format_Count, Surface_Formats);

   bool Desired_Format_Supported = false;
   VkSurfaceFormatKHR Desired_Format;

   for(u32 Format_Index = 0; Format_Index < Surface_Format_Count; ++Format_Index)
   {
      VkSurfaceFormatKHR Option = Surface_Formats[Format_Index];
      if(Option.format == VK_FORMAT_R8G8B8A8_SRGB && Option.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      {
         Desired_Format = Option;
         Desired_Format_Supported = true;
         break;
      }
   }
   Assert(Desired_Format_Supported);
   VK->Swapchain_Image_Format = Desired_Format.format;

   u32 Present_Mode_Count;
   vkGetPhysicalDeviceSurfacePresentModesKHR(VK->Physical_Device, VK->Surface, &Present_Mode_Count, 0);
   Assert(Present_Mode_Count > 0);

   VkPresentModeKHR *Present_Modes = Allocate(&VK->Scratch, VkPresentModeKHR, Present_Mode_Count);
   vkGetPhysicalDeviceSurfacePresentModesKHR(VK->Physical_Device, VK->Surface, &Present_Mode_Count, Present_Modes);

   VkPresentModeKHR Desired_Present_Mode = VK_PRESENT_MODE_FIFO_KHR;
   bool Desired_Present_Mode_Found = false;

   for(u32 Mode_Index = 0; Mode_Index < Present_Mode_Count; ++Mode_Index)
   {
      VkPresentModeKHR Option = Present_Modes[Mode_Index];
      if(Option == VK_PRESENT_MODE_MAILBOX_KHR)
      {
         Desired_Present_Mode = Option;
         Desired_Present_Mode_Found = true;
      }
   }
   // Assert(Desired_Present_Mode_Found);

   VK->Swapchain_Extent = Surface_Capabilities.currentExtent;
   if(VK->Swapchain_Extent.width == UINT32_MAX && VK->Swapchain_Extent.height == UINT32_MAX)
   {
      // NOTE: UINT32_MAX is a special case that allows us to specify our own
      // extent dimensions.
      int Width, Height;
      Get_Window_Dimensions(VK->Platform_Context, &Width, &Height);

      VK->Swapchain_Extent.width = Minimum(Maximum((u32)Width, Surface_Capabilities.minImageExtent.width), Surface_Capabilities.maxImageExtent.width);
      VK->Swapchain_Extent.height = Minimum(Maximum((u32)Height, Surface_Capabilities.minImageExtent.height), Surface_Capabilities.maxImageExtent.height);
   }

   u32 Image_Count = Surface_Capabilities.minImageCount + 1;
   if(Surface_Capabilities.maxImageCount && Surface_Capabilities.maxImageCount < Image_Count)
   {
      Image_Count = Surface_Capabilities.maxImageCount;
   }

   VkSwapchainCreateInfoKHR Swapchain_Info = {0};
   Swapchain_Info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
   Swapchain_Info.surface = VK->Surface;
   Swapchain_Info.minImageCount = Image_Count;
   Swapchain_Info.imageFormat = Desired_Format.format;
   Swapchain_Info.imageColorSpace = Desired_Format.colorSpace;
   Swapchain_Info.imageExtent = VK->Swapchain_Extent;
   Swapchain_Info.imageArrayLayers = 1;
   Swapchain_Info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

   u32 Swapchain_Queue_Family_Indices[] = {VK->Graphics_Queue_Family_Index, VK->Present_Queue_Family_Index};
   if(VK->Graphics_Queue_Family_Index == VK->Present_Queue_Family_Index)
   {
      Swapchain_Info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
   }
   else
   {
      Swapchain_Info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      Swapchain_Info.queueFamilyIndexCount = Array_Count(Swapchain_Queue_Family_Indices);
      Swapchain_Info.pQueueFamilyIndices = Swapchain_Queue_Family_Indices;
   }

   Swapchain_Info.preTransform = Surface_Capabilities.currentTransform;
   Swapchain_Info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
   Swapchain_Info.presentMode = Desired_Present_Mode;
   Swapchain_Info.clipped = VK_TRUE;
   Swapchain_Info.oldSwapchain = VK_NULL_HANDLE;

   VK_CHECK(vkCreateSwapchainKHR(VK->Device, &Swapchain_Info, 0, &VK->Swapchain));

   vkGetSwapchainImagesKHR(VK->Device, VK->Swapchain, &VK->Swapchain_Image_Count, 0);
   VK->Swapchain_Images = Allocate(&VK->Arena, VkImage, VK->Swapchain_Image_Count);
   vkGetSwapchainImagesKHR(VK->Device, VK->Swapchain, &VK->Swapchain_Image_Count, VK->Swapchain_Images);

   VK->Swapchain_Image_Views = Allocate(&VK->Arena, VkImageView, VK->Swapchain_Image_Count);
   for(u32 Image_Index = 0; Image_Index < VK->Swapchain_Image_Count; ++Image_Index)
   {
      VkImageViewCreateInfo View_Info = {0};
      View_Info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      View_Info.image = VK->Swapchain_Images[Image_Index];
      View_Info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      View_Info.format = VK->Swapchain_Image_Format;
      View_Info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      View_Info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      View_Info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      View_Info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      View_Info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      View_Info.subresourceRange.baseMipLevel = 0;
      View_Info.subresourceRange.levelCount = 1;
      View_Info.subresourceRange.baseArrayLayer = 0;
      View_Info.subresourceRange.layerCount = 1;

      VK_CHECK(vkCreateImageView(VK->Device, &View_Info, 0, &VK->Swapchain_Image_Views[Image_Index]));
   }
}

static void Create_Vulkan_Framebuffers(vulkan_context *VK)
{
   VK->Swapchain_Framebuffers = Allocate(&VK->Arena, VkFramebuffer, VK->Swapchain_Image_Count);
   for(u32 Image_Index = 0; Image_Index < VK->Swapchain_Image_Count; ++Image_Index)
   {
      VkImageView Attachments[] = {VK->Swapchain_Image_Views[Image_Index]};

      VkFramebufferCreateInfo Framebuffer_Info = {0};
      Framebuffer_Info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      Framebuffer_Info.renderPass = VK->Render_Pass;
      Framebuffer_Info.attachmentCount = 1;
      Framebuffer_Info.pAttachments = Attachments;
      Framebuffer_Info.width = VK->Swapchain_Extent.width;
      Framebuffer_Info.height = VK->Swapchain_Extent.height;
      Framebuffer_Info.layers = 1;

      VK_CHECK(vkCreateFramebuffer(VK->Device, &Framebuffer_Info, 0, VK->Swapchain_Framebuffers + Image_Index));
   }
}

static void Recreate_Vulkan_Swapchain(vulkan_context *VK)
{
   // TODO: Handle minimized windows with a width/height of zero.
   vkDeviceWaitIdle(VK->Device);

   Destroy_Vulkan_Swapchain(VK);
   Create_Vulkan_Swapchain(VK);
   Create_Vulkan_Framebuffers(VK);
}

static void Create_Vulkan_Buffer(vulkan_context *VK, VkBuffer *Buffer, VkDeviceMemory *Buffer_Memory, size Size, VkBufferUsageFlags Usage, VkMemoryPropertyFlags Properties)
{
   VkBufferCreateInfo Buffer_Info = {0};
   Buffer_Info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
   Buffer_Info.size = Size;
   Buffer_Info.usage = Usage;
   Buffer_Info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

   VK_CHECK(vkCreateBuffer(VK->Device, &Buffer_Info, 0, Buffer));

   VkMemoryRequirements Memory_Requirements;
   vkGetBufferMemoryRequirements(VK->Device, *Buffer, &Memory_Requirements);

   VkPhysicalDeviceMemoryProperties Memory_Properties;
   vkGetPhysicalDeviceMemoryProperties(VK->Physical_Device, &Memory_Properties);

   u32 Memory_Type_Index = 0;
   u32 Memory_Type_Found = false;
   for(u32 Type_Index = 0; Type_Index < Memory_Properties.memoryTypeCount; ++Type_Index)
   {
      VkMemoryType *Type = Memory_Properties.memoryTypes + Type_Index;

      u32 Type_Supported = Memory_Requirements.memoryTypeBits & (1 << Type_Index);
      u32 Properties_Supported = (Type->propertyFlags & Properties) == Properties;

      if(Type_Supported && Properties_Supported)
      {
         Memory_Type_Index = Type_Index;
         Memory_Type_Found = true;
         break;
      }
   }
   Assert(Memory_Type_Found);

   VkMemoryAllocateInfo Vertex_Allocate_Info = {0};
   Vertex_Allocate_Info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
   Vertex_Allocate_Info.allocationSize = Memory_Requirements.size;
   Vertex_Allocate_Info.memoryTypeIndex = Memory_Type_Index;

   VK_CHECK(vkAllocateMemory(VK->Device, &Vertex_Allocate_Info, 0, Buffer_Memory));
   VK_CHECK(vkBindBufferMemory(VK->Device, *Buffer, *Buffer_Memory, 0));
}

static void Copy_Vulkan_Buffer(vulkan_context *VK, VkBuffer Destination, VkBuffer Source, VkDeviceSize Size)
{
   VkCommandBufferAllocateInfo Allocate_Info = {0};
   Allocate_Info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   Allocate_Info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   Allocate_Info.commandPool = VK->Command_Pool;
   Allocate_Info.commandBufferCount = 1;

   VkCommandBuffer Command_Buffer;
   VK_CHECK(vkAllocateCommandBuffers(VK->Device, &Allocate_Info, &Command_Buffer));

   VkCommandBufferBeginInfo Begin_Info = {0};
   Begin_Info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
   Begin_Info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   VK_CHECK(vkBeginCommandBuffer(Command_Buffer, &Begin_Info));
   {
      VkBufferCopy Copy_Region = {0};
      Copy_Region.srcOffset = 0;
      Copy_Region.dstOffset = 0;
      Copy_Region.size = Size;
      vkCmdCopyBuffer(Command_Buffer, Source, Destination, 1, &Copy_Region);
   }
   vkEndCommandBuffer(Command_Buffer);

   VkSubmitInfo Submit_Info = {0};
   Submit_Info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   Submit_Info.commandBufferCount = 1;
   Submit_Info.pCommandBuffers = &Command_Buffer;

   vkQueueSubmit(VK->Graphics_Queue, 1, &Submit_Info, VK_NULL_HANDLE);
   vkQueueWaitIdle(VK->Graphics_Queue);

   vkFreeCommandBuffers(VK->Device, VK->Command_Pool, 1, &Command_Buffer);
}

static void Create_Vulkan_Vertex_Buffer(vulkan_context *VK, vertex *Vertices, size Size)
{
   VkBuffer Staging_Buffer;
   VkDeviceMemory Staging_Buffer_Memory;
   VkBufferUsageFlags Staging_Buffer_Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
   VkMemoryPropertyFlags Staging_Buffer_Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
   Create_Vulkan_Buffer(VK, &Staging_Buffer, &Staging_Buffer_Memory, Size, Staging_Buffer_Usage, Staging_Buffer_Properties);

   void *Data;
   VK_CHECK(vkMapMemory(VK->Device, Staging_Buffer_Memory, 0, Size, 0, &Data));
   memcpy(Data, Vertices, Size);
   vkUnmapMemory(VK->Device, Staging_Buffer_Memory);

   VkBufferUsageFlags Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
   VkMemoryPropertyFlags Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   Create_Vulkan_Buffer(VK, &VK->Vertex_Buffer, &VK->Vertex_Buffer_Memory, Size, Usage, Properties);

   Copy_Vulkan_Buffer(VK, VK->Vertex_Buffer, Staging_Buffer, Size);

   vkDestroyBuffer(VK->Device, Staging_Buffer, 0);
   vkFreeMemory(VK->Device, Staging_Buffer_Memory, 0);
}

static void Create_Vulkan_Index_Buffer(vulkan_context *VK, u16 *Indices, size Size)
{
   VkBuffer Staging_Buffer;
   VkDeviceMemory Staging_Buffer_Memory;
   VkBufferUsageFlags Staging_Buffer_Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
   VkMemoryPropertyFlags Staging_Buffer_Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
   Create_Vulkan_Buffer(VK, &Staging_Buffer, &Staging_Buffer_Memory, Size, Staging_Buffer_Usage, Staging_Buffer_Properties);

   void *Data;
   VK_CHECK(vkMapMemory(VK->Device, Staging_Buffer_Memory, 0, Size, 0, &Data));
   memcpy(Data, Indices, Size);
   vkUnmapMemory(VK->Device, Staging_Buffer_Memory);

   VkBufferUsageFlags Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
   VkMemoryPropertyFlags Properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
   Create_Vulkan_Buffer(VK, &VK->Index_Buffer, &VK->Index_Buffer_Memory, Size, Usage, Properties);

   Copy_Vulkan_Buffer(VK, VK->Index_Buffer, Staging_Buffer, Size);

   vkDestroyBuffer(VK->Device, Staging_Buffer, 0);
   vkFreeMemory(VK->Device, Staging_Buffer_Memory, 0);
}

static INITIALIZE_VULKAN(Initialize_Vulkan)
{
   VK->Platform_Context = Platform_Context;
   VK->Arena = Make_Arena(Megabytes(256));
   VK->Scratch = Make_Arena(Megabytes(256));

   // NOTE: Initialize the Vulkan instance.
   VkApplicationInfo Application_Info = {0};
   Application_Info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   Application_Info.pApplicationName = "Vulkan Renderer";
   Application_Info.applicationVersion = 1;
   Application_Info.apiVersion = VK_API_VERSION_1_0;

   const char *Instance_Extension_Names[] =
   {
      VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
      VK_KHR_SURFACE_EXTENSION_NAME,
      VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
   };
   Confirm_Vulkan_Instance_Extensions(Instance_Extension_Names, Array_Count(Instance_Extension_Names), VK->Scratch);

   VkInstanceCreateInfo Instance_Info = {0};
   Instance_Info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   Instance_Info.pApplicationInfo = &Application_Info;
   Instance_Info.enabledExtensionCount = Array_Count(Instance_Extension_Names);
   Instance_Info.ppEnabledExtensionNames = Instance_Extension_Names;
#if DEBUG
   const char *Required_Layer_Names[] = {"VK_LAYER_KHRONOS_validation"};
   Instance_Info.enabledLayerCount = Array_Count(Required_Layer_Names);
   Instance_Info.ppEnabledLayerNames = Required_Layer_Names;
#endif

   VK_CHECK(vkCreateInstance(&Instance_Info, 0, &VK->Instance));

   // NOTE: Choose a physical device.
   u32 Physical_Count = 0;
   VK_CHECK(vkEnumeratePhysicalDevices(VK->Instance, &Physical_Count, 0));
   Assert(Physical_Count > 0);

   VkPhysicalDevice *Physical_Devices = Allocate(&VK->Scratch, VkPhysicalDevice, Physical_Count);
   VK_CHECK(vkEnumeratePhysicalDevices(VK->Instance, &Physical_Count, Physical_Devices));

   VK->Physical_Device = VK_NULL_HANDLE;
   for(u32 Physical_Index = 0; Physical_Index < Physical_Count; ++Physical_Index)
   {
      VkPhysicalDevice Physical_Device = Physical_Devices[Physical_Index];

      VkPhysicalDeviceProperties Properties;
      vkGetPhysicalDeviceProperties(Physical_Device, &Properties);
      printf("Device Name: %s\n", Properties.deviceName);

      VkPhysicalDeviceFeatures Features;
      vkGetPhysicalDeviceFeatures(Physical_Device, &Features);

      // TODO: For now, just use the first encountered device. Later we'll query for
      // the most appropriate properties.
      if(VK->Physical_Device == VK_NULL_HANDLE)
      {
         VK->Physical_Device = Physical_Device;
      }
   }
   Assert(VK->Physical_Device != VK_NULL_HANDLE);

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
   wayland_context *Wayland = (wayland_context *)Platform_Context;

   VkWaylandSurfaceCreateInfoKHR Surface_Info = {0};
   Surface_Info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
   Surface_Info.display = Wayland->Display;
   Surface_Info.surface = Wayland->Surface;

   VK_CHECK(vkCreateWaylandSurfaceKHR(VK->Instance, &Surface_Info, 0, &VK->Surface));
#else
#  error Surface creation not yet implemented for this platform.
#endif

   // NOTE: Examine available queues.
   u32 Queue_Family_Count;
   vkGetPhysicalDeviceQueueFamilyProperties(VK->Physical_Device, &Queue_Family_Count, 0);

   VkQueueFamilyProperties *Queue_Families = Allocate(&VK->Scratch, VkQueueFamilyProperties, Queue_Family_Count);
   vkGetPhysicalDeviceQueueFamilyProperties(VK->Physical_Device, &Queue_Family_Count, Queue_Families);

   bool Compute_Queue_Family_Found = false;
   bool Graphics_Queue_Family_Found = false;
   bool Present_Queue_Family_Found = false;

   // NOTE: For now, the array length is just hard coded to the max number of
   // potential queue families we might have.
   u32 Queue_Info_Count = 0;
   VkDeviceQueueCreateInfo Queue_Infos[3];

   float Queue_Priorities[] = {1.0f};

   for(u32 Family_Index = 0; Family_Index < Queue_Family_Count; ++Family_Index)
   {
      VkQueueFamilyProperties Family = Queue_Families[Family_Index];
      bool Use_This_Family = false;

      if(Family.queueFlags & VK_QUEUE_COMPUTE_BIT)
      {
         Use_This_Family = true;
         Compute_Queue_Family_Found = true;
         VK->Compute_Queue_Family_Index = Family_Index;
      }

      if(Family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
         Use_This_Family = true;
         Graphics_Queue_Family_Found = true;
         VK->Graphics_Queue_Family_Index = Family_Index;
      }

      VkBool32 Present_Support;
      vkGetPhysicalDeviceSurfaceSupportKHR(VK->Physical_Device, Family_Index, VK->Surface, &Present_Support);
      if(Present_Support)
      {
         Use_This_Family = true;
         Present_Queue_Family_Found = true;
         VK->Present_Queue_Family_Index = Family_Index;
      }

      if(Use_This_Family)
      {
         // NOTE: Queue families specified when creating the device must be
         // unique, so just append creation info structs here. If we hit the
         // max, we know we found them all.
         if(Queue_Info_Count < Array_Count(Queue_Infos))
         {
            VkDeviceQueueCreateInfo Info = {0};
            Info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            Info.queueFamilyIndex = Family_Index;
            Info.queueCount = 1;
            Info.pQueuePriorities = Queue_Priorities;

            Queue_Infos[Queue_Info_Count++] = Info;
         }
         else
         {
            break;
         }
      }
   }
   Assert(Compute_Queue_Family_Found);
   Assert(Graphics_Queue_Family_Found);
   Assert(Present_Queue_Family_Found);

   // NOTE: Create the logical device.
   const char *Required_Device_Extension_Names[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
   Confirm_Vulkan_Device_Extensions(VK->Physical_Device, Required_Device_Extension_Names,
                                    Array_Count(Required_Device_Extension_Names), VK->Scratch);

   VkDeviceCreateInfo Device_Create_Info = {0};
   Device_Create_Info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   Device_Create_Info.queueCreateInfoCount = Queue_Info_Count;
   Device_Create_Info.pQueueCreateInfos = Queue_Infos;
   Device_Create_Info.enabledExtensionCount = Array_Count(Required_Device_Extension_Names);
   Device_Create_Info.ppEnabledExtensionNames = Required_Device_Extension_Names;

   VK_CHECK(vkCreateDevice(VK->Physical_Device, &Device_Create_Info, 0, &VK->Device));

   vkGetDeviceQueue(VK->Device, VK->Compute_Queue_Family_Index, 0, &VK->Compute_Queue);
   vkGetDeviceQueue(VK->Device, VK->Graphics_Queue_Family_Index, 0, &VK->Graphics_Queue);
   vkGetDeviceQueue(VK->Device, VK->Present_Queue_Family_Index, 0, &VK->Present_Queue);

   // NOTE: Initialize swap chain.
   Create_Vulkan_Swapchain(VK);

   // NOTE: Initialize shaders.
   string Vertex_Shader_Code = Read_Entire_File("basic_vertex.spv");
   string Fragment_Shader_Code = Read_Entire_File("basic_fragment.spv");

   VkShaderModuleCreateInfo Vertex_Shader_Info = {0};
   Vertex_Shader_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   Vertex_Shader_Info.codeSize = Vertex_Shader_Code.Length;
   Vertex_Shader_Info.pCode = (u32 *)Vertex_Shader_Code.Data;

   VkShaderModuleCreateInfo Fragment_Shader_Info = {0};
   Fragment_Shader_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   Fragment_Shader_Info.codeSize = Fragment_Shader_Code.Length;
   Fragment_Shader_Info.pCode = (u32 *)Fragment_Shader_Code.Data;

   VK_CHECK(vkCreateShaderModule(VK->Device, &Vertex_Shader_Info, 0, &VK->Vertex_Shader));
   VK_CHECK(vkCreateShaderModule(VK->Device, &Fragment_Shader_Info, 0, &VK->Fragment_Shader));

   VkPipelineShaderStageCreateInfo Vertex_Stage_Info = {0};
   Vertex_Stage_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   Vertex_Stage_Info.stage = VK_SHADER_STAGE_VERTEX_BIT;
   Vertex_Stage_Info.module = VK->Vertex_Shader;
   Vertex_Stage_Info.pName = "main";

   VkPipelineShaderStageCreateInfo Fragment_Stage_Info = {0};
   Fragment_Stage_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
   Fragment_Stage_Info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
   Fragment_Stage_Info.module = VK->Fragment_Shader;
   Fragment_Stage_Info.pName = "main";

   VkPipelineShaderStageCreateInfo Shader_Stage_Infos[] = {Vertex_Stage_Info, Fragment_Stage_Info};

   // NOTE: Configure pipeline inputs.
   VkVertexInputBindingDescription Vertex_Binding_Description = {0};
   Vertex_Binding_Description.binding = 0;
   Vertex_Binding_Description.stride = sizeof(*Vertices);
   Vertex_Binding_Description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

   VkVertexInputAttributeDescription Vertex_Attribute_Descriptions[2] = {0};
   Vertex_Attribute_Descriptions[0].binding = 0;
   Vertex_Attribute_Descriptions[0].location = 0;
   Vertex_Attribute_Descriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
   Vertex_Attribute_Descriptions[0].offset = offsetof(vertex, Position);
   Vertex_Attribute_Descriptions[1].binding = 0;
   Vertex_Attribute_Descriptions[1].location = 1;
   Vertex_Attribute_Descriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
   Vertex_Attribute_Descriptions[1].offset = offsetof(vertex, Color);

   VkPipelineVertexInputStateCreateInfo Vertex_Input_Info = {0};
   Vertex_Input_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   Vertex_Input_Info.vertexBindingDescriptionCount = 1;
   Vertex_Input_Info.pVertexBindingDescriptions = &Vertex_Binding_Description;
   Vertex_Input_Info.vertexAttributeDescriptionCount = Array_Count(Vertex_Attribute_Descriptions);
   Vertex_Input_Info.pVertexAttributeDescriptions = Vertex_Attribute_Descriptions;

   VkPipelineInputAssemblyStateCreateInfo Input_Assembly_Info = {0};
   Input_Assembly_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
   Input_Assembly_Info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   Input_Assembly_Info.primitiveRestartEnable = VK_FALSE;

   // NOTE: Set up viewport/scissor.
   VkViewport Viewport = {0};
   Viewport.width = (float)VK->Swapchain_Extent.width;
   Viewport.height = (float)VK->Swapchain_Extent.height;
   Viewport.minDepth = 0.0f;
   Viewport.maxDepth = 1.0f;

   VkRect2D Scissor = {0};
   Scissor.extent = VK->Swapchain_Extent;

   VkDynamicState Dynamic_States[] =
   {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
   };
   VkPipelineDynamicStateCreateInfo Dynamic_Info = {0};
   Dynamic_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
   Dynamic_Info.dynamicStateCount = Array_Count(Dynamic_States);
   Dynamic_Info.pDynamicStates = Dynamic_States;

   VkPipelineViewportStateCreateInfo Viewport_Info = {0};
   Viewport_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
   Viewport_Info.viewportCount = 1;
   Viewport_Info.scissorCount = 1;

   // NOTE: Configure rasterizer.
   VkPipelineRasterizationStateCreateInfo Rasterizer_Info = {0};
   Rasterizer_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
   Rasterizer_Info.depthClampEnable = VK_FALSE;
   Rasterizer_Info.polygonMode = VK_POLYGON_MODE_FILL;
   Rasterizer_Info.lineWidth = 1.0f;
   Rasterizer_Info.cullMode = VK_CULL_MODE_BACK_BIT;
   Rasterizer_Info.frontFace = VK_FRONT_FACE_CLOCKWISE;
   Rasterizer_Info.depthBiasEnable = VK_FALSE;
   Rasterizer_Info.depthBiasConstantFactor = 0.0f;
   Rasterizer_Info.depthBiasClamp = 0.0f;
   Rasterizer_Info.depthBiasSlopeFactor = 0.0f;

   // NOTE: Configure multisampling.
   VkPipelineMultisampleStateCreateInfo Multisample_Info = {0};
   Multisample_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
   Multisample_Info.sampleShadingEnable = VK_FALSE;
   Multisample_Info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
   Multisample_Info.minSampleShading = 1.0f;
   Multisample_Info.pSampleMask = 0;
   Multisample_Info.alphaToCoverageEnable = VK_FALSE;
   Multisample_Info.alphaToOneEnable = VK_FALSE;

   // NOTE: Configure color blending.
   VkPipelineColorBlendAttachmentState Blend_Attachment = {0};
   Blend_Attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT|VK_COLOR_COMPONENT_G_BIT|VK_COLOR_COMPONENT_B_BIT|VK_COLOR_COMPONENT_A_BIT;
   Blend_Attachment.blendEnable = VK_FALSE;
   Blend_Attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
   Blend_Attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
   Blend_Attachment.colorBlendOp = VK_BLEND_OP_ADD;
   Blend_Attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
   Blend_Attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
   Blend_Attachment.alphaBlendOp = VK_BLEND_OP_ADD;

   VkPipelineColorBlendStateCreateInfo Blend_Info = {0};
   Blend_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
   Blend_Info.logicOpEnable = VK_FALSE;
   Blend_Info.logicOp = VK_LOGIC_OP_COPY;
   Blend_Info.attachmentCount = 1;
   Blend_Info.pAttachments = &Blend_Attachment;
   Blend_Info.blendConstants[0] = 0.0f;
   Blend_Info.blendConstants[1] = 0.0f;
   Blend_Info.blendConstants[2] = 0.0f;
   Blend_Info.blendConstants[3] = 0.0f;

   // NOTE: Create command buffers.
   VkCommandPoolCreateInfo Pool_Info = {0};
   Pool_Info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
   Pool_Info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
   Pool_Info.queueFamilyIndex = VK->Graphics_Queue_Family_Index;

   VK_CHECK(vkCreateCommandPool(VK->Device, &Pool_Info, 0, &VK->Command_Pool));

   VkCommandBufferAllocateInfo Allocate_Info = {0};
   Allocate_Info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
   Allocate_Info.commandPool = VK->Command_Pool;
   Allocate_Info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
   Allocate_Info.commandBufferCount = Array_Count(VK->Command_Buffers);

   VK_CHECK(vkAllocateCommandBuffers(VK->Device, &Allocate_Info, VK->Command_Buffers));

   // NOTE: Create buffers.
   Create_Vulkan_Vertex_Buffer(VK, Vertices, sizeof(Vertices));
   Create_Vulkan_Index_Buffer(VK, Indices, sizeof(Indices));
   for(int Frame_Index = 0; Frame_Index < MAX_FRAMES_IN_FLIGHT; ++Frame_Index)
   {
      VkBuffer *Buffer = VK->Uniform_Buffers + Frame_Index;
      VkDeviceMemory *Memory = VK->Uniform_Buffer_Memories + Frame_Index;
      size Size = sizeof(uniform_buffer_object);
      VkBufferUsageFlags Usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
      VkMemoryPropertyFlags Properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
      Create_Vulkan_Buffer(VK, Buffer, Memory, Size, Usage, Properties);

      void *Mapped = VK->Mapped_Uniform_Buffers + Frame_Index;
      VK_CHECK(vkMapMemory(VK->Device, *Memory, 0, Size, 0, Mapped));
   }

   // NOTE: Create descriptor set layout.
   VkDescriptorSetLayoutBinding UBO_Layout_Binding = {0};
   UBO_Layout_Binding.binding = 0;
   UBO_Layout_Binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   UBO_Layout_Binding.descriptorCount = 1;
   UBO_Layout_Binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
   UBO_Layout_Binding.pImmutableSamplers = 0;

   VkDescriptorSetLayoutCreateInfo Descriptor_Layout_Info = {0};
   Descriptor_Layout_Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
   Descriptor_Layout_Info.bindingCount = 1;
   Descriptor_Layout_Info.pBindings = &UBO_Layout_Binding;
   VK_CHECK(vkCreateDescriptorSetLayout(VK->Device, &Descriptor_Layout_Info, 0, &VK->Descriptor_Set_Layout));

   VkDescriptorPoolSize Descriptor_Pool_Size = {0};
   Descriptor_Pool_Size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
   Descriptor_Pool_Size.descriptorCount = MAX_FRAMES_IN_FLIGHT;

   VkDescriptorPoolCreateInfo Descriptor_Pool_Info = {0};
   Descriptor_Pool_Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
   Descriptor_Pool_Info.poolSizeCount = 1;
   Descriptor_Pool_Info.pPoolSizes = &Descriptor_Pool_Size;
   Descriptor_Pool_Info.maxSets = MAX_FRAMES_IN_FLIGHT;

   VK_CHECK(vkCreateDescriptorPool(VK->Device, &Descriptor_Pool_Info, 0, &VK->Descriptor_Pool));

   VkDescriptorSetLayout Descriptor_Set_Layouts[MAX_FRAMES_IN_FLIGHT];
   for(int Frame_Index = 0; Frame_Index < MAX_FRAMES_IN_FLIGHT; ++Frame_Index)
   {
      Descriptor_Set_Layouts[Frame_Index] = VK->Descriptor_Set_Layout;
   }
   VkDescriptorSetAllocateInfo Descriptor_Set_Info = {0};
   Descriptor_Set_Info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
   Descriptor_Set_Info.descriptorPool = VK->Descriptor_Pool;
   Descriptor_Set_Info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
   Descriptor_Set_Info.pSetLayouts = Descriptor_Set_Layouts;

   VK_CHECK(vkAllocateDescriptorSets(VK->Device, &Descriptor_Set_Info, VK->Descriptor_Sets));

   // NOTE: Create descriptor sets.
   for(int Frame_Index = 0; Frame_Index < MAX_FRAMES_IN_FLIGHT; ++Frame_Index)
   {
      VkDescriptorBufferInfo Buffer_Info = {0};
      Buffer_Info.buffer = VK->Uniform_Buffers[Frame_Index];
      Buffer_Info.offset = 0;
      Buffer_Info.range = sizeof(uniform_buffer_object);

      VkWriteDescriptorSet Descriptor_Write = {0};
      Descriptor_Write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      Descriptor_Write.dstSet = VK->Descriptor_Sets[Frame_Index];
      Descriptor_Write.dstBinding = 0;
      Descriptor_Write.dstArrayElement = 0;
      Descriptor_Write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
      Descriptor_Write.descriptorCount = 1;
      Descriptor_Write.pBufferInfo = &Buffer_Info;
      Descriptor_Write.pImageInfo = 0;
      Descriptor_Write.pTexelBufferView = 0;

      vkUpdateDescriptorSets(VK->Device, 1, &Descriptor_Write, 0, 0);
   }

   // NOTE: Create pipeline layout.
   VkPipelineLayoutCreateInfo Layout_Info = {0};
   Layout_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   Layout_Info.setLayoutCount = 1;
   Layout_Info.pSetLayouts = &VK->Descriptor_Set_Layout;
   Layout_Info.pushConstantRangeCount = 0;
   Layout_Info.pPushConstantRanges = 0;

   VK_CHECK(vkCreatePipelineLayout(VK->Device, &Layout_Info, 0, &VK->Pipeline_Layout));

   // NOTE: Initialize render pass.
   VkAttachmentDescription Color_Attachment = {0};
   Color_Attachment.format = VK->Swapchain_Image_Format;
   Color_Attachment.samples = VK_SAMPLE_COUNT_1_BIT;
   Color_Attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
   Color_Attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
   Color_Attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
   Color_Attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
   Color_Attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
   Color_Attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

   VkAttachmentReference Color_Attachment_Reference = {0};
   Color_Attachment_Reference.attachment = 0;
   Color_Attachment_Reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

   VkSubpassDescription Subpass = {0};
   Subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
   Subpass.colorAttachmentCount = 1;
   Subpass.pColorAttachments = &Color_Attachment_Reference;

   VkSubpassDependency Subpass_Dependency = {0};
   Subpass_Dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
   Subpass_Dependency.dstSubpass = 0;
   Subpass_Dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   Subpass_Dependency.srcAccessMask = 0;
   Subpass_Dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
   Subpass_Dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

   VkRenderPassCreateInfo Render_Pass_Info = {0};
   Render_Pass_Info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   Render_Pass_Info.attachmentCount = 1;
   Render_Pass_Info.pAttachments = &Color_Attachment;
   Render_Pass_Info.subpassCount = 1;
   Render_Pass_Info.pSubpasses = &Subpass;
   Render_Pass_Info.dependencyCount = 1;
   Render_Pass_Info.pDependencies = &Subpass_Dependency;

   VK_CHECK(vkCreateRenderPass(VK->Device, &Render_Pass_Info, 0, &VK->Render_Pass));

   VkGraphicsPipelineCreateInfo Pipeline_Info = {0};
   Pipeline_Info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
   Pipeline_Info.stageCount = 2;
   Pipeline_Info.pStages = Shader_Stage_Infos;
   Pipeline_Info.pVertexInputState = &Vertex_Input_Info;
   Pipeline_Info.pInputAssemblyState = &Input_Assembly_Info;
   Pipeline_Info.pViewportState = &Viewport_Info;
   Pipeline_Info.pRasterizationState = &Rasterizer_Info;
   Pipeline_Info.pMultisampleState = &Multisample_Info;
   Pipeline_Info.pDepthStencilState = 0;
   Pipeline_Info.pColorBlendState = &Blend_Info;
   Pipeline_Info.pDynamicState = &Dynamic_Info;
   Pipeline_Info.layout = VK->Pipeline_Layout;
   Pipeline_Info.renderPass = VK->Render_Pass;
   Pipeline_Info.subpass = 0;
   Pipeline_Info.basePipelineHandle = VK_NULL_HANDLE;
   Pipeline_Info.basePipelineIndex = -1;

   VK_CHECK(vkCreateGraphicsPipelines(VK->Device, VK_NULL_HANDLE, 1, &Pipeline_Info, 0, &VK->Graphics_Pipeline));

   // NOTE: Configure synchronization.
   for(int Frame_Index = 0; Frame_Index < MAX_FRAMES_IN_FLIGHT; ++Frame_Index)
   {
      VkSemaphoreCreateInfo Semaphore_Info = {0};
      Semaphore_Info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
      VK_CHECK(vkCreateSemaphore(VK->Device, &Semaphore_Info, 0, VK->Image_Available_Semaphores + Frame_Index));
      VK_CHECK(vkCreateSemaphore(VK->Device, &Semaphore_Info, 0, VK->Render_Finished_Semaphores + Frame_Index));

      VkFenceCreateInfo Fence_Info = {0};
      Fence_Info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      Fence_Info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
      VK_CHECK(vkCreateFence(VK->Device, &Fence_Info, 0, VK->In_Flight_Fences + Frame_Index));
   }

   // NOTE: Create framebuffer.
   Create_Vulkan_Framebuffers(VK);

   // NOTE: Clear temporary memory.
   Reset_Arena(&VK->Scratch);
}

static RENDER_WITH_VULKAN(Render_With_Vulkan)
{
   VkFence *In_Flight_Fence = VK->In_Flight_Fences + VK->Frame_Index;
   vkWaitForFences(VK->Device, 1, In_Flight_Fence, VK_TRUE, UINT64_MAX);

   u32 Image_Index;
   VkResult Image_Acquisition_Result = vkAcquireNextImageKHR(VK->Device, VK->Swapchain, UINT64_MAX, VK->Image_Available_Semaphores[VK->Frame_Index], VK_NULL_HANDLE, &Image_Index);
   if(Image_Acquisition_Result == VK_ERROR_OUT_OF_DATE_KHR)
   {
      Recreate_Vulkan_Swapchain(VK);
   }
   else
   {
      VK_CHECK(Image_Acquisition_Result);
      vkResetFences(VK->Device, 1, In_Flight_Fence);

      VkCommandBuffer Command_Buffer = VK->Command_Buffers[VK->Frame_Index];
      vkResetCommandBuffer(Command_Buffer, 0);

      // NOTE: Record render commands.
      VkCommandBufferBeginInfo Buffer_Begin_Info = {0};
      Buffer_Begin_Info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      Buffer_Begin_Info.flags = 0;
      Buffer_Begin_Info.pInheritanceInfo = 0;

      VK_CHECK(vkBeginCommandBuffer(Command_Buffer, &Buffer_Begin_Info));

      VkRenderPassBeginInfo Pass_Begin_Info = {0};
      Pass_Begin_Info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
      Pass_Begin_Info.renderPass = VK->Render_Pass;
      Pass_Begin_Info.framebuffer = VK->Swapchain_Framebuffers[Image_Index];
      Pass_Begin_Info.renderArea.extent = VK->Swapchain_Extent;
      VkClearValue Clear_Color = {{{0, 0, 1, 1}}};
      Pass_Begin_Info.clearValueCount = 1;
      Pass_Begin_Info.pClearValues = &Clear_Color;

      vkCmdBeginRenderPass(Command_Buffer, &Pass_Begin_Info, VK_SUBPASS_CONTENTS_INLINE);
      {
         vkCmdBindPipeline(Command_Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VK->Graphics_Pipeline);

         VkBuffer Vertex_Buffers[] = {VK->Vertex_Buffer};
         VkDeviceSize Vertex_Buffer_Offsets[] = {0};
         vkCmdBindVertexBuffers(Command_Buffer, 0, 1, Vertex_Buffers, Vertex_Buffer_Offsets);
         vkCmdBindIndexBuffer(Command_Buffer, VK->Index_Buffer, 0, VK_INDEX_TYPE_UINT16);

         VkViewport Viewport = {0};
         Viewport.x = 0.0f;
         Viewport.y = 0.0f;
         Viewport.width = (float)VK->Swapchain_Extent.width;
         Viewport.height = (float)VK->Swapchain_Extent.height;
         Viewport.minDepth = 0.0f;
         Viewport.maxDepth = 1.0f;
         vkCmdSetViewport(Command_Buffer, 0, 1, &Viewport);

         VkRect2D Scissor = {0};
         Scissor.extent = VK->Swapchain_Extent;
         vkCmdSetScissor(Command_Buffer, 0, 1, &Scissor);

         vkCmdBindDescriptorSets(Command_Buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, VK->Pipeline_Layout, 0, 1, VK->Descriptor_Sets + VK->Frame_Index, 0, 0);

         vkCmdDrawIndexed(Command_Buffer, Array_Count(Indices), 1, 0, 0, 0);
      }
      vkCmdEndRenderPass(Command_Buffer);
      VK_CHECK(vkEndCommandBuffer(Command_Buffer));

      // NOTE: Update uniforms.
      static float Delta;
      vec3 Eye = {5, 5, 5};
      vec3 Target = {0, 0, 0};

      uniform_buffer_object UBO = {0};
      UBO.Model = Rotate_Y(Cosine(Delta));
      UBO.View = Look_At(Eye, Target);
      UBO.Projection = Perspective(VK->Swapchain_Extent.width, VK->Swapchain_Extent.height, 0.1f, 100.0f);

      memcpy(VK->Mapped_Uniform_Buffers[VK->Frame_Index], &UBO, sizeof(UBO));

      Delta += 0.000016;
      if(Delta >= 1.0f) Delta -= 1.0f;

      // NOTE: Submit command buffer.
      VkSubmitInfo Submit_Info = {0};
      Submit_Info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

      VkSemaphore Wait_Semaphores[] = {VK->Image_Available_Semaphores[VK->Frame_Index]};
      VkPipelineStageFlags Wait_Stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
      Assert(Array_Count(Wait_Semaphores) == Array_Count(Wait_Stages));

      Submit_Info.waitSemaphoreCount = Array_Count(Wait_Semaphores);
      Submit_Info.pWaitSemaphores = Wait_Semaphores;
      Submit_Info.pWaitDstStageMask = Wait_Stages;
      Submit_Info.commandBufferCount = 1;
      Submit_Info.pCommandBuffers = VK->Command_Buffers + VK->Frame_Index;

      VkSemaphore Signal_Semaphores[] = {VK->Render_Finished_Semaphores[VK->Frame_Index]};
      Submit_Info.signalSemaphoreCount = Array_Count(Signal_Semaphores);
      Submit_Info.pSignalSemaphores = Signal_Semaphores;

      VK_CHECK(vkQueueSubmit(VK->Graphics_Queue, 1, &Submit_Info, *In_Flight_Fence));

      VkPresentInfoKHR Present_Info = {0};
      Present_Info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
      Present_Info.waitSemaphoreCount = 1;
      Present_Info.pWaitSemaphores = Signal_Semaphores;

      VkSwapchainKHR Swapchains[] = {VK->Swapchain};
      Present_Info.swapchainCount = 1;
      Present_Info.pSwapchains = Swapchains;
      Present_Info.pImageIndices = &Image_Index;
      Present_Info.pResults = 0;

      VkResult Present_Result = vkQueuePresentKHR(VK->Present_Queue, &Present_Info);
      if(Present_Result == VK_ERROR_OUT_OF_DATE_KHR || Present_Result == VK_SUBOPTIMAL_KHR || VK->Framebuffer_Resized)
      {
         VK->Framebuffer_Resized = false;
         Recreate_Vulkan_Swapchain(VK);
      }
      else
      {
         VK_CHECK(Present_Result);
      }

      VK->Frame_Index++;
      VK->Frame_Index %= MAX_FRAMES_IN_FLIGHT;
      Reset_Arena(&VK->Scratch);
   }
}

static DESTROY_VULKAN(Destroy_Vulkan)
{
   vkDeviceWaitIdle(VK->Device);
   for(int Frame_Index = 0; Frame_Index < MAX_FRAMES_IN_FLIGHT; ++Frame_Index)
   {
      vkDestroySemaphore(VK->Device, VK->Render_Finished_Semaphores[Frame_Index], 0);
      vkDestroySemaphore(VK->Device, VK->Image_Available_Semaphores[Frame_Index], 0);
      vkDestroyFence(VK->Device, VK->In_Flight_Fences[Frame_Index], 0);
   }

   Destroy_Vulkan_Swapchain(VK);
   vkDestroyCommandPool(VK->Device, VK->Command_Pool, 0);

   for(int Frame_Index = 0; Frame_Index < MAX_FRAMES_IN_FLIGHT; ++Frame_Index)
   {
      vkDestroyBuffer(VK->Device, VK->Uniform_Buffers[Frame_Index], 0);
      vkFreeMemory(VK->Device, VK->Uniform_Buffer_Memories[Frame_Index], 0);
   }

   vkDestroyBuffer(VK->Device, VK->Index_Buffer, 0);
   vkFreeMemory(VK->Device, VK->Index_Buffer_Memory, 0);

   vkDestroyBuffer(VK->Device, VK->Vertex_Buffer, 0);
   vkFreeMemory(VK->Device, VK->Vertex_Buffer_Memory, 0);

   vkDestroyPipeline(VK->Device, VK->Graphics_Pipeline, 0);
   vkDestroyPipelineLayout(VK->Device, VK->Pipeline_Layout, 0);
   vkDestroyDescriptorPool(VK->Device, VK->Descriptor_Pool, 0);
   vkDestroyDescriptorSetLayout(VK->Device, VK->Descriptor_Set_Layout, 0);
   vkDestroyRenderPass(VK->Device, VK->Render_Pass, 0);
   vkDestroyShaderModule(VK->Device, VK->Fragment_Shader, 0);
   vkDestroyShaderModule(VK->Device, VK->Vertex_Shader, 0);

   vkDestroyDevice(VK->Device, 0);
   vkDestroySurfaceKHR(VK->Instance, VK->Surface, 0);
   vkDestroyInstance(VK->Instance, 0);
}
