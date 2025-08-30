/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: This file is the entry point for the Xlib-based Linux build. Each
// supported platform will have its code constrained to a single file, which
// will quarantine the underlying platform from the renderer.

#define VK_USE_PLATFORM_XLIB_KHR
#include <vulkan/vulkan.h>

#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <time.h>

#include "shared.h"
#include "platform.h"
#include "vulkan_renderer.h"

typedef struct {
   Display *Display;
   Window Window;

   vulkan_context VK;
   bool Running;
} xlib_context;

#include "vulkan_renderer.c"

// NOTE: Platform API implementations.
#include "platform_linux.c"

static GET_WINDOW_DIMENSIONS(Get_Window_Dimensions)
{
   xlib_context *Xlib = Platform_Context;

   XWindowAttributes Window_Attributes = {0};
   XGetWindowAttributes(Xlib->Display, Xlib->Window, &Window_Attributes);

   *Width  = (int)Window_Attributes.width;
   *Height = (int)Window_Attributes.height;
}

static void Toggle_Xlib_Fullscreen(xlib_context *Xlib)
{
   Atom WM_State = XInternAtom(Xlib->Display, "_NET_WM_STATE", False);
   Atom WM_Fullscreen = XInternAtom(Xlib->Display, "_NET_WM_STATE_FULLSCREEN", False);

   XEvent Event = {0};
   Event.type = ClientMessage;
   Event.xclient.window = Xlib->Window;
   Event.xclient.message_type = WM_State;
   Event.xclient.format = 32;
   Event.xclient.data.l[0] = 2; // NOTE: 0 = remove, 1 = add, 2 = toggle
   Event.xclient.data.l[1] = WM_Fullscreen;
   Event.xclient.data.l[2] = 0;

   long Mask = SubstructureRedirectMask|SubstructureNotifyMask;
   XSendEvent(Xlib->Display, DefaultRootWindow(Xlib->Display), False, Mask, &Event);
   XFlush(Xlib->Display);
}

static void Initialize_Xlib(xlib_context *Xlib, int Width, int Height)
{
   Xlib->Display = XOpenDisplay(0);

   int Screen = DefaultScreen(Xlib->Display);
   int Depth = DefaultDepth(Xlib->Display, Screen);
   Visual *Visual = DefaultVisual(Xlib->Display, Screen);
   Window Root = RootWindow(Xlib->Display, Screen);

   XSetWindowAttributes Window_Attributes = {0};
   u64 Attribute_Mask = 0;

   Window_Attributes.background_pixel = 0;
   Attribute_Mask |= CWBackPixel;

   Window_Attributes.border_pixel = 0;
   Attribute_Mask |= CWBorderPixel;

   // NOTE(law): Seeting the bit gravity to StaticGravity prevents flickering
   // during window resize.
   Window_Attributes.bit_gravity = StaticGravity;
   Attribute_Mask |= CWBitGravity;

   Window_Attributes.event_mask = 0;
   Window_Attributes.event_mask |= ExposureMask;
   Window_Attributes.event_mask |= KeyPressMask;
   Window_Attributes.event_mask |= KeyReleaseMask;
   Window_Attributes.event_mask |= ButtonPressMask;
   Window_Attributes.event_mask |= ButtonReleaseMask;
   Window_Attributes.event_mask |= StructureNotifyMask;
   Window_Attributes.event_mask |= PropertyChangeMask;
   Attribute_Mask |= CWEventMask;

   Xlib->Window = XCreateWindow(Xlib->Display, Root, 0, 0, Width, Height, 0, Depth, InputOutput, Visual, Attribute_Mask, &Window_Attributes);
   if(!Xlib->Window)
   {
      Log("Failed to create X window.\n");
   }
   else
   {
      XStoreName(Xlib->Display, Xlib->Window, "Vulkan Renderer (Xlib)");

      XSizeHints Size_Hints = {0};
      Size_Hints.flags = PMinSize|PMaxSize;
      Size_Hints.min_width = Width / 2;
      Size_Hints.min_height = Height / 2;
      Size_Hints.max_width = Width;
      Size_Hints.max_height = Height;
      XSetWMNormalHints(Xlib->Display, Xlib->Window, &Size_Hints);

      XMapWindow(Xlib->Display, Xlib->Window);
      XFlush(Xlib->Display);

      Xlib->Running = true;
   }
}

static void Process_Xlib_Events(xlib_context *Xlib)
{
   while(Xlib->Running && XPending(Xlib->Display))
   {
      XEvent Event;
      XNextEvent(Xlib->Display, &Event);

      // NOTE: Prevent key repeating.
      if(Event.type == KeyRelease && XEventsQueued(Xlib->Display, QueuedAfterReading))
      {
         XEvent Next_Event;
         XPeekEvent(Xlib->Display, &Next_Event);
         if(Next_Event.type == KeyPress &&
            Next_Event.xkey.time == Event.xkey.time &&
            Next_Event.xkey.keycode == Event.xkey.keycode)
         {
            XNextEvent(Xlib->Display, &Event);
            continue;
         }
      }

      switch(Event.type)
      {
         case DestroyNotify:
         {
            XDestroyWindowEvent Destroy_Notify_Event = Event.xdestroywindow;
            if(Destroy_Notify_Event.window == Xlib->Window)
            {
               Xlib->Running = false;
            }
         } break;

         case Expose:
         {
            XExposeEvent Expose_Event = Event.xexpose;
            if(Expose_Event.count != 0)
            {
               continue;
            }
         } break;

         case ConfigureNotify:
         {
            // TODO: Handle resizing the window.
            // int Window_Width  = Event.xconfigure.width;
            // int Window_Height = Event.xconfigure.height;
         } break;

         case KeyPress:
         case KeyRelease:
         {
            XKeyEvent Key_Event = Event.xkey;

            bool Pressed = (Event.type == KeyPress);
            bool Alt_Pressed = (Key_Event.state|XK_Meta_L|XK_Meta_R);

            KeySym Key;
            char Characters[256];
            XLookupString(&Key_Event, Characters, sizeof(Characters), &Key, 0);

            switch(Key)
            {
               case XK_f:
               case XK_F11: {
                  if(Pressed)
                  {
                     Toggle_Xlib_Fullscreen(Xlib);
                  }
               } break;

               case XK_Return: {
                  if(Pressed && Alt_Pressed)
                  {
                     Toggle_Xlib_Fullscreen(Xlib);
                  }
               } break;

               case XK_Escape: {
                  Xlib->Running = false;
               } break;
            }
         } break;

         case ButtonPress:
         case ButtonRelease:
         {
         } break;
      }
   }
}

int main(void)
{
   xlib_context Xlib = {0};
   Initialize_Xlib(&Xlib, DEFAULT_RESOLUTION_WIDTH, DEFAULT_RESOLUTION_HEIGHT);

   if(Initialize_Vulkan(&Xlib.VK, &Xlib))
   {
      struct timespec Frame_Start = {0};
      struct timespec Frame_End = {0};

      while(Xlib.Running)
      {
         // NOTE: Handle any received input events.
         Process_Xlib_Events(&Xlib);

         // NOTE: Computing the frame time is only meaningful while we have
         // vsync active via VK_PRESENT_MODE_FIFO_KHR.
         float Frame_Seconds_Elapsed = Compute_Seconds_Elapsed(&Frame_Start, &Frame_End);

         // NOTE: Perform actual rendering work.
         Render_With_Vulkan(&Xlib.VK, Frame_Seconds_Elapsed);
      }

      Destroy_Vulkan(&Xlib.VK);
   }

   XCloseDisplay(Xlib.Display);

   return(0);
}
