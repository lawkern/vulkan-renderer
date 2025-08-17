/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#define VK_CHECK(Result)                                                \
   do {                                                                 \
      VkResult Err = (Result);                                          \
      if(Err != VK_SUCCESS)                                             \
      {                                                                 \
         fprintf(stderr, "Vulkan Error: %s\n", string_VkResult(Err));   \
         Invalid_Code_Path;                                             \
      }                                                                 \
   } while(0)


static bool Vulkan_Extensions_Supported(VkExtensionProperties *Extensions, u32 Extension_Count,
                                        const char **Required_Names, u32 Required_Count)
{
   bool Result = true;
   for(u32 Required_Index = 0; Required_Index < Required_Count; ++Required_Index)
   {
      const char *Required_Name = Required_Names[Required_Index];
      bool Found = false;
      for(u32 Available_Index = 0; Available_Index < Extension_Count; ++Available_Index)
      {
         if(strcmp(Required_Name, Extensions[Available_Index].extensionName) == 0)
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

static INITIALIZE_VULKAN(Initialize_Vulkan)
{
   arena Arena = Make_Arena(Megabytes(256));

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
   Confirm_Vulkan_Instance_Extensions(Instance_Extension_Names, Array_Count(Instance_Extension_Names), Arena);

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

   VkPhysicalDevice *Physical_Devices = Allocate(&Arena, VkPhysicalDevice, Physical_Count);
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

      VkPhysicalDeviceMemoryProperties Memory_Properties;
      vkGetPhysicalDeviceMemoryProperties(Physical_Device, &Memory_Properties);

      for(u32 Heap_Index = 0; Heap_Index < Memory_Properties.memoryHeapCount; ++Heap_Index)
      {
         VkMemoryHeap *Heap = Memory_Properties.memoryHeaps + Heap_Index;
         printf("  Heap %d Size: %lu\n", Heap_Index, Heap->size);
      }

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

   VkQueueFamilyProperties *Queue_Families = Allocate(&Arena, VkQueueFamilyProperties, Queue_Family_Count);
   vkGetPhysicalDeviceQueueFamilyProperties(VK->Physical_Device, &Queue_Family_Count, Queue_Families);

   bool Compute_Queue_Family_Found = false;
   bool Graphics_Queue_Family_Found = false;
   bool Present_Queue_Family_Found = false;

   u32 Compute_Queue_Family_Index;
   u32 Graphics_Queue_Family_Index;
   u32 Present_Queue_Family_Index;

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
         Compute_Queue_Family_Index = Family_Index;
      }

      if(Family.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      {
         Use_This_Family = true;
         Graphics_Queue_Family_Found = true;
         Graphics_Queue_Family_Index = Family_Index;
      }

      VkBool32 Present_Support;
      vkGetPhysicalDeviceSurfaceSupportKHR(VK->Physical_Device, Family_Index, VK->Surface, &Present_Support);
      if(Present_Support)
      {
         Use_This_Family = true;
         Present_Queue_Family_Found = true;
         Present_Queue_Family_Index = Family_Index;
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
                                    Array_Count(Required_Device_Extension_Names), Arena);

   VkDeviceCreateInfo Device_Create_Info = {0};
   Device_Create_Info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   Device_Create_Info.queueCreateInfoCount = Queue_Info_Count;
   Device_Create_Info.pQueueCreateInfos = Queue_Infos;
   Device_Create_Info.enabledExtensionCount = Array_Count(Required_Device_Extension_Names);
   Device_Create_Info.ppEnabledExtensionNames = Required_Device_Extension_Names;

   VK_CHECK(vkCreateDevice(VK->Physical_Device, &Device_Create_Info, 0, &VK->Device));

   vkGetDeviceQueue(VK->Device, Compute_Queue_Family_Index, 0, &VK->Compute_Queue);
   vkGetDeviceQueue(VK->Device, Graphics_Queue_Family_Index, 0, &VK->Graphics_Queue);
   vkGetDeviceQueue(VK->Device, Present_Queue_Family_Index, 0, &VK->Present_Queue);

   // NOTE: Initialize swap chain.
   VkSurfaceCapabilitiesKHR Surface_Capabilities;
   vkGetPhysicalDeviceSurfaceCapabilitiesKHR(VK->Physical_Device, VK->Surface, &Surface_Capabilities);

   u32 Surface_Format_Count;
   vkGetPhysicalDeviceSurfaceFormatsKHR(VK->Physical_Device, VK->Surface, &Surface_Format_Count, 0);
   Assert(Surface_Format_Count > 0);

   VkSurfaceFormatKHR *Surface_Formats = Allocate(&Arena, VkSurfaceFormatKHR, Surface_Format_Count);
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

   VkPresentModeKHR *Present_Modes = Allocate(&Arena, VkPresentModeKHR, Present_Mode_Count);
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
      // extent dimesnsions.
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

   u32 Swapchain_Queue_Family_Indices[] = {Graphics_Queue_Family_Index, Present_Queue_Family_Index};
   if(Graphics_Queue_Family_Index == Present_Queue_Family_Index)
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
   VK->Swapchain_Images = Allocate(&Arena, VkImage, VK->Swapchain_Image_Count);
   vkGetSwapchainImagesKHR(VK->Device, VK->Swapchain, &VK->Swapchain_Image_Count, VK->Swapchain_Images);

   VK->Swapchain_Image_Views = Allocate(&Arena, VkImageView, VK->Swapchain_Image_Count);
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

   // NOTE: Initialize shaders.
   platform_file Vertex_Shader_Code = Read_Entire_File("shaders/basic_vertex.spv");
   platform_file Fragment_Shader_Code = Read_Entire_File("shaders/basic_fragment.spv");

   VkShaderModuleCreateInfo Vertex_Shader_Info = {0};
   Vertex_Shader_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   Vertex_Shader_Info.codeSize = Vertex_Shader_Code.Size;
   Vertex_Shader_Info.pCode = (u32 *)Vertex_Shader_Code.Memory;

   VkShaderModuleCreateInfo Fragment_Shader_Info = {0};
   Fragment_Shader_Info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
   Fragment_Shader_Info.codeSize = Fragment_Shader_Code.Size;
   Fragment_Shader_Info.pCode = (u32 *)Fragment_Shader_Code.Memory;

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
   VkPipelineVertexInputStateCreateInfo Vertex_Input_Info = {0};
   Vertex_Input_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
   Vertex_Input_Info.vertexBindingDescriptionCount = 0;
   Vertex_Input_Info.pVertexBindingDescriptions = 0;
   Vertex_Input_Info.vertexAttributeDescriptionCount = 0;
   Vertex_Input_Info.pVertexAttributeDescriptions = 0;

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

   // NOTE: Create pipeline layout.
   VkPipelineLayoutCreateInfo Layout_Info = {0};
   Layout_Info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
   Layout_Info.setLayoutCount = 0;
   Layout_Info.pSetLayouts = 0;
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

   VkRenderPassCreateInfo Render_Pass_Info = {0};
   Render_Pass_Info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
   Render_Pass_Info.attachmentCount = 1;
   Render_Pass_Info.pAttachments = &Color_Attachment;
   Render_Pass_Info.subpassCount = 1;
   Render_Pass_Info.pSubpasses = &Subpass;

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
}

static RENDER_WITH_VULKAN(Render_With_Vulkan)
{
   (void)VK;
}

static DESTROY_VULKAN(Destroy_Vulkan)
{
   vkDestroyPipeline(VK->Device, VK->Graphics_Pipeline, 0);
   vkDestroyPipelineLayout(VK->Device, VK->Pipeline_Layout, 0);
   vkDestroyRenderPass(VK->Device, VK->Render_Pass, 0);
   vkDestroyPipelineLayout(VK->Device, VK->Pipeline_Layout, 0);
   vkDestroyShaderModule(VK->Device, VK->Fragment_Shader, 0);
   vkDestroyShaderModule(VK->Device, VK->Vertex_Shader, 0);
   for(u32 Image_Index = 0; Image_Index < VK->Swapchain_Image_Count; ++Image_Index)
   {
      vkDestroyImageView(VK->Device, VK->Swapchain_Image_Views[Image_Index], 0);
   }
   vkDestroySwapchainKHR(VK->Device, VK->Swapchain, 0);
   vkDestroyDevice(VK->Device, 0);
   vkDestroyInstance(VK->Instance, 0);
}
