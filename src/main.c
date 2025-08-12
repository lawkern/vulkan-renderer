/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#include <stdio.h>
#include "shared.h"

#define VK_CHECK(Result)                                                \
   do {                                                                 \
      VkResult Err = (Result);                                          \
      if(Err != VK_SUCCESS)                                             \
      {                                                                 \
         fprintf(stderr, "Vulkan Error: %s\n", string_VkResult(Err));   \
         Invalid_Code_Path;                                             \
      }                                                                 \
   } while(0)

int main(void)
{
   arena Arena = Make_Arena(Megabytes(256));

   // NOTE: Initialize the Vulkan instance.
   VkApplicationInfo Application_Info = {0};
   Application_Info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
   Application_Info.pApplicationName = "Vulkan Renderer";
   Application_Info.applicationVersion = 1;
   Application_Info.apiVersion = VK_API_VERSION_1_0;

   VkInstanceCreateInfo Instance_Info = {0};
   Instance_Info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
   Instance_Info.pApplicationInfo = &Application_Info;

   // TODO: Define and use our own allocator.
   VkAllocationCallbacks Allocator = {0};
   (void)Allocator;

   VkInstance Instance;
   VK_CHECK(vkCreateInstance(&Instance_Info, 0, &Instance));

   // NOTE: Choose a physical device.
   u32 Physical_Count = 0;
   VK_CHECK(vkEnumeratePhysicalDevices(Instance, &Physical_Count, 0));

   Assert(Physical_Count > 0);

   VkPhysicalDevice *Physical_Devices = Allocate(&Arena, VkPhysicalDevice, Physical_Count);
   VK_CHECK(vkEnumeratePhysicalDevices(Instance, &Physical_Count, Physical_Devices));

   // TODO: For now, just use the last encountered device. Later we'll query for
   // the most appropriate properties.
   VkPhysicalDevice Physical_Device;
   for(u32 Physical_Index = 0; Physical_Index < Physical_Count; ++Physical_Index)
   {
      Physical_Device = Physical_Devices[Physical_Index];

      VkPhysicalDeviceProperties Properties;
      vkGetPhysicalDeviceProperties(Physical_Device, &Properties);

      printf("Device Name: %s\n", Properties.deviceName);
   }

   u32 Queue_Family_Property_Count;
   vkGetPhysicalDeviceQueueFamilyProperties(Physical_Device, &Queue_Family_Property_Count, 0);
   printf("Queue Family Count: %d\n", Queue_Family_Property_Count);

   VkQueueFamilyProperties Queue_Properties;
   vkGetPhysicalDeviceQueueFamilyProperties(Physical_Device, &Queue_Family_Property_Count, &Queue_Properties);

   Assert(Queue_Properties.queueFlags & VK_QUEUE_GRAPHICS_BIT);

   VkDeviceQueueCreateInfo Queue_Create_Info = {0};
   Queue_Create_Info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
   Queue_Create_Info.queueFamilyIndex = 0;
   Queue_Create_Info.queueCount = 1;

   VkDeviceCreateInfo Device_Create_Info = {0};
   Device_Create_Info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
   Device_Create_Info.queueCreateInfoCount = 1;
   Device_Create_Info.pQueueCreateInfos = &Queue_Create_Info;

   VkDevice Device;
   VK_CHECK(vkCreateDevice(Physical_Device, &Device_Create_Info, 0, &Device));

   return(0);
}
