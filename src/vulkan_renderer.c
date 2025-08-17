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
}

static RENDER_WITH_VULKAN(Render_With_Vulkan)
{
   (void)VK;
}

static DESTROY_VULKAN(Destroy_Vulkan)
{
   vkDestroyDevice(VK->Device, 0);
   vkDestroyInstance(VK->Instance, 0);
}
