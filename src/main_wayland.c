/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: This file is the entry point for the Wayland-based Linux build. Each
// supported platform will have its code constrained to a single file, which
// will quarantine the underlying platform from the renderer.

#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#include <fcntl.h>
#include <linux/input-event-codes.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdio.h>

#include "external/xdg-shell-client-protocol.h"
#include "external/xdg-shell-protocol.c"

#include "external/xdg-decoration-unstable-v1-client-protocol.h"
#include "external/xdg-decoration-unstable-v1-protocol.c"

#include "shared.h"
#include "platform.h"
#include "vulkan_renderer.h"

#define SHM_BUFFER_COUNT 2

typedef struct {
   struct wl_display *Display;
   struct wl_compositor *Compositor;
   struct wl_surface *Surface;
   struct wl_registry *Registry;
   struct wl_seat *Seat;
   struct wl_keyboard *Keyboard;

   struct wl_shm_pool *Pool;
   struct wl_buffer *Buffers[SHM_BUFFER_COUNT];
   struct wl_shm *Shared_Memory;
   size Shared_Memory_Size;
   int Shared_Memory_File;
   int Buffer_Index;
   u32 *Buffer_Pixels;

   struct xdg_wm_base *Desktop_Base;
   struct xdg_surface *Desktop_Surface;
   struct xdg_toplevel *Desktop_Toplevel;

   int Window_Width;
   int Window_Height;
   int Previous_Window_Width;
   int Previous_Window_Height;

   u64 Frame_Count;
   struct timespec Frame_Start;
   struct timespec Frame_End;

   bool Running;
   bool Alt_Pressed;

   struct zxdg_decoration_manager_v1 *Decoration_Manager;
   struct zxdg_toplevel_decoration_v1 *Toplevel_Decoration;

   vulkan_context VK;
} wayland_context;

#include "vulkan_renderer.c"

// NOTE: Platform API implementations.
#include "platform_linux.c"

static GET_WINDOW_DIMENSIONS(Get_Window_Dimensions)
{
   wayland_context *Wayland = Platform_Context;

   *Width = Wayland->Window_Width;
   *Height = Wayland->Window_Height;
}

static inline void Toggle_Wayland_Fullscreen(wayland_context *Wayland)
{
   static bool Fullscreen;
   if(!Fullscreen)
   {
      xdg_toplevel_set_fullscreen(Wayland->Desktop_Toplevel, 0);
   }
   else
   {
      xdg_toplevel_unset_fullscreen(Wayland->Desktop_Toplevel);
   }
   Fullscreen = !Fullscreen;
}

static int Create_Wayland_Shared_Memory_File(size Size)
{
   char Template[] = "/tmp/wayland-XXXXXX";

   int Result = mkstemp(Template);
   if(Result >= 0)
   {
      unlink(Template);
      if(ftruncate(Result, Size) == 0)
      {
         // NOTE: Success.
      }
      else
      {
         close(Result);

         Log("Failed to truncate shared memory file.\n");
         Invalid_Code_Path;
      }
   }
   else
   {
      Log("Failed to open shared memory file.\n");
      Invalid_Code_Path;
   }

   return(Result);
}

static void Destroy_Wayland_Pixel_Buffers(wayland_context *Wayland)
{
   for(int Buffer_Index = 0; Buffer_Index < SHM_BUFFER_COUNT; ++Buffer_Index)
   {
      if(Wayland->Buffers[Buffer_Index])
      {
         wl_surface_attach(Wayland->Surface, 0, 0, 0);
         wl_surface_commit(Wayland->Surface);
         wl_buffer_destroy(Wayland->Buffers[Buffer_Index]);
         Wayland->Buffers[Buffer_Index] = 0;
      }
   }
   if(Wayland->Pool)
   {
      wl_shm_pool_destroy(Wayland->Pool);
      Wayland->Pool = 0;
   }
   if(Wayland->Buffer_Pixels)
   {
      munmap(Wayland->Buffer_Pixels, Wayland->Shared_Memory_Size);
      Wayland->Buffer_Pixels = 0;
   }
   if(Wayland->Shared_Memory_File > 0)
   {
      // NOTE: We're treating 0 as unitialized despite -1 maybe being more
      // appropriate. In practice, if it's bigger than the std stream values
      // we're probably good to close it.
      close(Wayland->Shared_Memory_File);
      Wayland->Shared_Memory_File = 0;
   }
}

static void Create_Wayland_Pixel_Buffers(wayland_context *Wayland, int Width, int Height)
{
   Destroy_Wayland_Pixel_Buffers(Wayland);

   int Stride = Width * sizeof(u32);
   Wayland->Shared_Memory_Size = Stride * Height * SHM_BUFFER_COUNT;
   Wayland->Shared_Memory_File = Create_Wayland_Shared_Memory_File(Wayland->Shared_Memory_Size);

   Wayland->Buffer_Pixels = mmap(0, Wayland->Shared_Memory_Size, PROT_READ|PROT_WRITE, MAP_SHARED, Wayland->Shared_Memory_File, 0);
   Wayland->Pool = wl_shm_create_pool(Wayland->Shared_Memory, Wayland->Shared_Memory_File, Wayland->Shared_Memory_Size);
   for(int Buffer_Index = 0; Buffer_Index < SHM_BUFFER_COUNT; ++Buffer_Index)
   {
      size Offset = Stride * Height * Buffer_Index;
      Wayland->Buffers[Buffer_Index] = wl_shm_pool_create_buffer(Wayland->Pool, Offset, Width, Height, Stride, WL_SHM_FORMAT_XRGB8888);

      wl_surface_attach(Wayland->Surface, Wayland->Buffers[Buffer_Index], 0, 0);
   }
   wl_surface_damage(Wayland->Surface, 0, 0, Width, Height);
   wl_surface_commit(Wayland->Surface);
}

// NOTE: Configure desktop base callbacks.
static void Ping_Desktop_Base(void *Data, struct xdg_wm_base *Base, u32 Serial)
{
   xdg_wm_base_pong(Base, Serial);
}
static const struct xdg_wm_base_listener Desktop_Base_Listener =
{
   .ping = Ping_Desktop_Base,
};

// NOTE: Configure desktop surface callbacks.
static void Configure_Desktop_Surface(void *Data, struct xdg_surface *Surface, u32 Serial)
{
   xdg_surface_ack_configure(Surface, Serial);

   wayland_context *Wayland = Data;
   if(Wayland->Window_Width != Wayland->Previous_Window_Width ||
      Wayland->Window_Height != Wayland->Previous_Window_Height)
   {
      Wayland->VK.Resize_Requested = true;
      Wayland->Previous_Window_Width = Wayland->Window_Width;
      Wayland->Previous_Window_Height = Wayland->Window_Height;
   }
}
static const struct xdg_surface_listener Desktop_Surface_Listener =
{
   .configure = Configure_Desktop_Surface,
};

// NOTE: Configure desktop toplevel callbacks.
static void Configure_Desktop_Toplevel(void *Data, struct xdg_toplevel *Toplevel, s32 Width, s32 Height, struct wl_array *States)
{
   wayland_context *Wayland = Data;
   if(Width > 0 && Height > 0)
   {
      Wayland->Window_Width = Width;
      Wayland->Window_Height = Height;
   }
   else
   {
      Wayland->Window_Width = DEFAULT_RESOLUTION_WIDTH;
      Wayland->Window_Height = DEFAULT_RESOLUTION_HEIGHT;
   }
}
static void Close_Desktop_Toplevel(void *Data, struct xdg_toplevel *Toplevel)
{
   wayland_context *Wayland = Data;
   Wayland->Running = false;
}
static const struct xdg_toplevel_listener Desktop_Toplevel_Listener =
{
   .configure = Configure_Desktop_Toplevel,
   .close = Close_Desktop_Toplevel,
};

// NOTE: Configure keyboard input callbacks.
static void Map_Keyboard(void *Data, struct wl_keyboard *Keyboard, u32 Format, int File, u32 Size)
{
   close(File);
}
static void Enter_Keyboard(void *Data, struct wl_keyboard *Keyboard, u32 Serial, struct wl_surface *Surface, struct wl_array *Keys)
{
   // ...
}
static void Leave_Keyboard(void *Data, struct wl_keyboard *Keyboard, u32 Serial, struct wl_surface *Surface)
{
   // ...
}
static void Key_Keyboard(void *Data, struct wl_keyboard *Keyboard, u32 Serial, u32 Time, u32 Key, u32 State)
{
   wayland_context *Wayland = (wayland_context *)Data;
   bool Pressed = (State == WL_KEYBOARD_KEY_STATE_PRESSED);

   switch(Key)
   {
      case KEY_F:
      case KEY_F11: {
         if(Pressed)
         {
            Toggle_Wayland_Fullscreen(Wayland);
         }
      } break;

      case KEY_ENTER: {
         if(Pressed && Wayland->Alt_Pressed)
         {
            Toggle_Wayland_Fullscreen(Wayland);
         }
      } break;

      case KEY_ESC: {
         if(Pressed)
         {
            Wayland->Running = false;
         }
      } break;
   }
}
static void Modify_Keyboard(void *Data, struct wl_keyboard *Keyboard, u32 Serial, u32 Mods_Depressed, u32 Mods_Latched, u32 Mods_Locked, u32 Group)
{
   wayland_context *Wayland = (wayland_context *)Data;

   // TODO: There are more robust options than hard-coding 3 for Alt. They're
   // much more verbose though.
   u32 Alt_Bit = (1 << 3);
   Wayland->Alt_Pressed = (Mods_Depressed & Alt_Bit);
}
static void Repeat_Keyboard(void *Data, struct wl_keyboard *Keyboard, int Rate, int Delay)
{
   // ...
}
static const struct wl_keyboard_listener Keyboard_Listener =
{
   .keymap = Map_Keyboard,
   .enter = Enter_Keyboard,
   .leave = Leave_Keyboard,
   .key = Key_Keyboard,
   .modifiers = Modify_Keyboard,
   .repeat_info = Repeat_Keyboard,
};

// NOTE: Configure frame completion.
static void Create_Wayland_Frame_Callback(wayland_context *Wayland);

static void Frame_Done(void *Data, struct wl_callback *Callback, u32 Time)
{
   wayland_context *Wayland = Data;

   wl_callback_destroy(Callback);
   Create_Wayland_Frame_Callback(Wayland);
}
static const struct wl_callback_listener Frame_Listener =
{
   .done = Frame_Done,
};

static void Create_Wayland_Frame_Callback(wayland_context *Wayland)
{
   // NOTE: If we cared about fullscreen for the fallback layer, we could
   // reallocate the buffer or mess with the viewport. For now, just drawing
   // within the orginal dimensions is good enough.
#if 0
   int Width = Wayland->Window_Width;
   int Height = Wayland->Window_Height;
#else
   int Width = DEFAULT_RESOLUTION_WIDTH;
   int Height = DEFAULT_RESOLUTION_HEIGHT;
#endif
   u32 *Pixels = Wayland->Buffer_Pixels + Width*Height*Wayland->Buffer_Index;

   Display_Simple_Debug_Message(Pixels, Width, Height);

   wl_surface_attach(Wayland->Surface, Wayland->Buffers[Wayland->Buffer_Index], 0, 0);
   wl_surface_damage(Wayland->Surface, 0, 0, Width, Height);

   Wayland->Buffer_Index++;
   Wayland->Buffer_Index %= SHM_BUFFER_COUNT;

   // NOTE: This is here to print the frame time, the value isn't important.
   Compute_Seconds_Elapsed(&Wayland->Frame_Start, &Wayland->Frame_End);

   struct wl_callback *Next_Callback = wl_surface_frame(Wayland->Surface);
   wl_callback_add_listener(Next_Callback, &Frame_Listener, Wayland);

   wl_surface_commit(Wayland->Surface);
}

// NOTE: Configure the global registry.
static void Global_Registry(void *Data, struct wl_registry *Registry, u32 ID, const char *Interface, u32 Version)
{
   // Log("%s v%u: %u\n", Interface, Version, ID);

   wayland_context *Wayland = Data;
   if(C_Strings_Are_Equal(Interface, wl_compositor_interface.name))
   {
      Wayland->Compositor = wl_registry_bind(Registry, ID, &wl_compositor_interface, 4);
   }
   else if(C_Strings_Are_Equal(Interface, xdg_wm_base_interface.name))
   {
      Wayland->Desktop_Base = wl_registry_bind(Registry, ID, &xdg_wm_base_interface, 1);
   }
   else if(C_Strings_Are_Equal(Interface, zxdg_decoration_manager_v1_interface.name))
   {
      Wayland->Decoration_Manager = wl_registry_bind(Registry, ID, &zxdg_decoration_manager_v1_interface, 1);
   }
   else if(C_Strings_Are_Equal(Interface, wl_seat_interface.name))
   {
      Wayland->Seat = wl_registry_bind(Registry, ID, &wl_seat_interface, 1);
      if(Wayland->Seat)
      {
         // NOTE: It's not the end of the world if we don't have keyboard
         // access.
         Wayland->Keyboard = wl_seat_get_keyboard(Wayland->Seat);
         if(Wayland->Keyboard)
         {
            wl_keyboard_add_listener(Wayland->Keyboard, &Keyboard_Listener, Wayland);
         }
      }
   }
   else if(C_Strings_Are_Equal(Interface, wl_shm_interface.name))
   {
      Wayland->Shared_Memory = wl_registry_bind(Registry, ID, &wl_shm_interface, 1);
   }
}
static void Remove_Global_Registry(void *Data, struct wl_registry *Registry, u32 ID)
{
   // ...
}
static const struct wl_registry_listener Registry_Listener =
{
   .global = Global_Registry,
   .global_remove = Remove_Global_Registry,
};

static void Destroy_Wayland(wayland_context *Wayland)
{
   // NOTE: Destroy shared memory.
   Destroy_Wayland_Pixel_Buffers(Wayland);
   if(Wayland->Shared_Memory)
   {
      wl_shm_destroy(Wayland->Shared_Memory);
      Wayland->Shared_Memory = 0;
   }

   // NOTE: Destroy decorations.
   if(Wayland->Toplevel_Decoration)
   {
      zxdg_toplevel_decoration_v1_destroy(Wayland->Toplevel_Decoration);
   }

   // NOTE: Destroy desktop.
   if(Wayland->Desktop_Toplevel)
   {
      xdg_toplevel_destroy(Wayland->Desktop_Toplevel);
   }
   if(Wayland->Desktop_Surface)
   {
      xdg_surface_destroy(Wayland->Desktop_Surface);
   }
   if(Wayland->Desktop_Base)
   {
      xdg_wm_base_destroy(Wayland->Desktop_Base);
   }

   // NOTE: Destroy Wayland.
   if(Wayland->Keyboard)
   {
      wl_keyboard_destroy(Wayland->Keyboard);
   }
   if(Wayland->Seat)
   {
      wl_seat_destroy(Wayland->Seat);
   }
   if(Wayland->Surface)
   {
      wl_surface_destroy(Wayland->Surface);
   }
   if(Wayland->Compositor)
   {
      wl_compositor_destroy(Wayland->Compositor);
   }
   if(Wayland->Registry)
   {
      wl_registry_destroy(Wayland->Registry);
   }
   if(Wayland->Display)
   {
      wl_display_disconnect(Wayland->Display);
   }
}

static void Initialize_Wayland(wayland_context *Wayland, int Width, int Height)
{
   Wayland->Display = wl_display_connect(0);
   if(Wayland->Display)
   {
      Wayland->Registry = wl_display_get_registry(Wayland->Display);
      wl_registry_add_listener(Wayland->Registry, &Registry_Listener, Wayland);

      wl_display_dispatch(Wayland->Display);
      wl_display_roundtrip(Wayland->Display);

      if(Wayland->Compositor && Wayland->Desktop_Base)
      {
         xdg_wm_base_add_listener(Wayland->Desktop_Base, &Desktop_Base_Listener, Wayland);

         Wayland->Surface = wl_compositor_create_surface(Wayland->Compositor);
         if(Wayland->Surface)
         {
            Wayland->Desktop_Surface = xdg_wm_base_get_xdg_surface(Wayland->Desktop_Base, Wayland->Surface);
            if(Wayland->Desktop_Surface)
            {
               xdg_surface_add_listener(Wayland->Desktop_Surface, &Desktop_Surface_Listener, Wayland);

               Wayland->Desktop_Toplevel = xdg_surface_get_toplevel(Wayland->Desktop_Surface);
               if(Wayland->Desktop_Toplevel)
               {
                  xdg_toplevel_add_listener(Wayland->Desktop_Toplevel, &Desktop_Toplevel_Listener, Wayland);
                  xdg_toplevel_set_title(Wayland->Desktop_Toplevel, "Vulkan Window (Wayland)");

                  // NOTE: Explicitly setting the min and max sizes to the same
                  // values is an indirect way of requesting a floating window
                  // in Wayland, since only the compositor gets to decide what
                  // actually happens.
                  xdg_toplevel_set_min_size(Wayland->Desktop_Toplevel, Width, Height);
                  xdg_toplevel_set_max_size(Wayland->Desktop_Toplevel, Width, Height);

                  // NOTE: Unclear why decorations are considered unstable, but
                  // if we can't get a title bar it's not a big deal.
                  if(Wayland->Decoration_Manager)
                  {
                     Wayland->Toplevel_Decoration = zxdg_decoration_manager_v1_get_toplevel_decoration(Wayland->Decoration_Manager, Wayland->Desktop_Toplevel);
                     zxdg_toplevel_decoration_v1_set_mode(Wayland->Toplevel_Decoration, ZXDG_TOPLEVEL_DECORATION_V1_MODE_SERVER_SIDE);
                  }

                  wl_surface_commit(Wayland->Surface);
                  wl_display_dispatch(Wayland->Display);

                  // NOTE: Set up a fallback buffer to draw into if we fail to
                  // initialize Vulkan.
                  Create_Wayland_Pixel_Buffers(Wayland, Width, Height);

                  Wayland->Running = true;
               }
               else
               {
                  Log("Failed to create XDG toplevel\n");
               }
            }
            else
            {
               Log("Failed to create XDG surface\n");
            }
         }
         else
         {
            Log("Failed to create Wayland surface\n");
         }
      }
      else
      {
         Log("Failed to bind required Wayland interfaces\n");
      }
   }
   else
   {
      Log("Failed to connect to Wayland display\n");
   }
}

int main(void)
{
   wayland_context Wayland = {0};
   Initialize_Wayland(&Wayland, DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT);

   if(Initialize_Vulkan(&Wayland.VK, &Wayland))
   {
      while(Wayland.Running)
      {
         // NOTE: Handle any received input events.
         wl_display_dispatch_pending(Wayland.Display);

         // NOTE: Computing the frame time is only meaningful while we have
         // vsync active via VK_PRESENT_MODE_FIFO_KHR.
         float Frame_Seconds_Elapsed = Compute_Seconds_Elapsed(&Wayland.Frame_Start, &Wayland.Frame_End);

         // NOTE: Perform actual rendering work.
         Render_With_Vulkan(&Wayland.VK, Frame_Seconds_Elapsed);
      }

      Destroy_Vulkan(&Wayland.VK);
   }
   else
   {
      // NOTE: This is our fallback path to display a simple error message in
      // the case we fail to initialize Vulkan.
      Create_Wayland_Frame_Callback(&Wayland);
      while(Wayland.Running)
      {
         // NOTE: We still need to handle input events in the main loop, even if
         // rendering is happening in the frame callback.
         wl_display_dispatch(Wayland.Display);
      }
   }

   Destroy_Wayland(&Wayland);

   return(0);
}
