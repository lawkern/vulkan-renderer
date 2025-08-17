/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: This file is the entry point for the Wayland-based Linux build. Each
// supported platform will have its code constrained to a single file, which
// will quarantine the underlying platform from the renderer.

#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <linux/input-event-codes.h>

#include <stdbool.h>
#include <stdio.h>

#include "external/xdg-shell-client-protocol.h"
#include "external/xdg-shell-protocol.c"

#include "external/xdg-decoration-unstable-v1-client-protocol.h"
#include "external/xdg-decoration-unstable-v1-protocol.c"

typedef struct {
   struct wl_display *Display;
   struct wl_compositor *Compositor;
   struct wl_surface *Surface;
   struct wl_registry *Registry;
   struct wl_seat *Seat;
   struct wl_keyboard *Keyboard;

   struct xdg_wm_base *Desktop_Base;
   struct xdg_surface *Desktop_Surface;
   struct xdg_toplevel *Desktop_Toplevel;

   int Window_Width;
   int Window_Height;
   bool Running;
   bool Alt_Pressed;

   struct zxdg_decoration_manager_v1 *Decoration_Manager;
   struct zxdg_toplevel_decoration_v1 *Toplevel_Decoration;
} wayland_context;

#include "shared.h"
#include "vulkan_renderer.h"
#include "vulkan_renderer.c"

// NOTE: Configure the global registry and its callbacks.
static void Global_Registry(void *Data, struct wl_registry *Registry, u32 ID, const char *Interface, u32 Version)
{
   wayland_context *Wayland = Data;
   if(Strings_Are_Equal(Interface, wl_compositor_interface.name))
   {
      Wayland->Compositor = wl_registry_bind(Registry, ID, &wl_compositor_interface, 4);
   }
   else if(Strings_Are_Equal(Interface, xdg_wm_base_interface.name))
   {
      Wayland->Desktop_Base = wl_registry_bind(Registry, ID, &xdg_wm_base_interface, 1);
   }
   else if(Strings_Are_Equal(Interface, zxdg_decoration_manager_v1_interface.name))
   {
      Wayland->Decoration_Manager = wl_registry_bind(Registry, ID, &zxdg_decoration_manager_v1_interface, 1);
   }
   else if(Strings_Are_Equal(Interface, wl_seat_interface.name))
   {
      Wayland->Seat = wl_registry_bind(Registry, ID, &wl_seat_interface, 1);
   }
}
static void Remove_Global_Registry(void *Data, struct wl_registry *Registry, u32 ID)
{
}
static const struct wl_registry_listener Registry_Listener =
{
   .global = Global_Registry,
   .global_remove = Remove_Global_Registry,
};

// NOTE: Configure desktop base callbacks.
static void Ping_Desktop_Base(void *Data, struct xdg_wm_base *Base, u32 Serial)
{
   xdg_wm_base_pong(Base, Serial);
}
static const struct xdg_wm_base_listener Desktop_Base_Listener =
{
   .ping = Ping_Desktop_Base,
};

static void Configure_Desktop_Surface(void *Data, struct xdg_surface *Surface, u32 Serial)
{
   xdg_surface_ack_configure(Surface, Serial);
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

static void Toggle_Wayland_Fullscreen(wayland_context *Wayland)
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

// NOTE: Configure keyboard input callbacks.
static void Map_Keyboard(void *Data, struct wl_keyboard *Keyboard, u32 Format, int File, u32 Size)
{
   close(File);
}
static void Enter_Keyboard(void *Data, struct wl_keyboard *Keyboard, u32 Serial, struct wl_surface *Surface, struct wl_array *Keys)
{
}
static void Leave_Keyboard(void *Data, struct wl_keyboard *Keyboard, u32 Serial, struct wl_surface *Surface)
{
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

static void Destroy_Wayland(wayland_context *Wayland)
{
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
         // NOTE: It's not the end of the world if we don't have keyboard
         // access. I'm still undecided on how much of this kind of
         // initialization should happen in the callbacks vs. procedurally
         // here. Not sure which is more "idiomatic Wayland", or if I care.
         if(Wayland->Seat)
         {
            Wayland->Keyboard = wl_seat_get_keyboard(Wayland->Seat);
            if(Wayland->Keyboard)
            {
               wl_keyboard_add_listener(Wayland->Keyboard, &Keyboard_Listener, Wayland);
            }
         }

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
                  xdg_toplevel_set_title(Wayland->Desktop_Toplevel, "Vulkan Window");

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

                  Wayland->Running = true;
               }
               else
               {
                  fprintf(stderr, "Failed to create XDG toplevel\n");
               }
            }
            else
            {
               fprintf(stderr, "Failed to create XDG surface\n");
            }
         }
         else
         {
            fprintf(stderr, "Failed to create Wayland surface\n");
         }
      }
      else
      {
         fprintf(stderr, "Failed to bind required Wayland interfaces\n");
      }
   }
   else
   {
      fprintf(stderr, "Failed to connect to Wayland display\n");
   }
}

int main(void)
{
   wayland_context Wayland = {0};
   Initialize_Wayland(&Wayland, 640, 480);

   vulkan_context VK = {0};
   Initialize_Vulkan(&VK, 640, 480, &Wayland);

   while(Wayland.Running)
   {
      wl_display_dispatch_pending(Wayland.Display);

      Render_With_Vulkan(&VK);

      wl_display_flush(Wayland.Display);
   }

   Destroy_Vulkan(&VK);
   Destroy_Wayland(&Wayland);

   return(0);
}
