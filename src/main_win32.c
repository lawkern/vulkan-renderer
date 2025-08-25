/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: This file is the entry point for the Win32-based Windows build. Each
// supported platform will have its code constrained to a single file, which
// will quarantine the underlying platform from the renderer.

#include <windows.h>
#include "shared.h"
#include "platform.h"

typedef struct {
   HWND Window;
   bool Running;
} win32_context;

static READ_ENTIRE_FILE(Read_Entire_File)
{
   string Result = {0};
   return(Result);
}

static void Get_Win32_Window_Dimensions(HWND Window, int *Width, int *Height)
{
   RECT Client_Rect;
   GetClientRect(Window, &Client_Rect);

   *Width  = Client_Rect.right - Client_Rect.left;
   *Height  = Client_Rect.bottom - Client_Rect.top;
}

static GET_WINDOW_DIMENSIONS(Get_Window_Dimensions)
{
   win32_context *Win32 = Platform_Context;
   Get_Win32_Window_Dimensions(Win32->Window, Width, Height);
}

static bool Is_Win32_Fullscreen(HWND Window)
{
   DWORD Style = GetWindowLong(Window, GWL_STYLE);

   bool Result = !(Style & WS_OVERLAPPEDWINDOW);
   return(Result);
}

static void Toggle_Win32_Fullscreen(HWND Window)
{
   // NOTE(law): Based on version by Raymond Chen:
   // https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353

  static WINDOWPLACEMENT Previous_Window_Placement = {sizeof(Previous_Window_Placement)};

   // TODO(law): Check what this does with multiple monitors.
   DWORD Style = GetWindowLong(Window, GWL_STYLE);
   if(Style & WS_OVERLAPPEDWINDOW)
   {
      MONITORINFO Monitor_Info = {sizeof(Monitor_Info)};

      if(GetWindowPlacement(Window, &Previous_Window_Placement) &&
         GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &Monitor_Info))
      {
         int X = Monitor_Info.rcMonitor.left;
         int Y = Monitor_Info.rcMonitor.top;
         int Width = Monitor_Info.rcMonitor.right - Monitor_Info.rcMonitor.left;
         int Height = Monitor_Info.rcMonitor.bottom - Monitor_Info.rcMonitor.top;

         SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
         SetWindowPos(Window, HWND_TOP, X, Y, Width, Height, SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
      }
   }
   else
   {
      SetWindowLong(Window, GWL_STYLE, Style|WS_OVERLAPPEDWINDOW);
      SetWindowPlacement(Window, &Previous_Window_Placement);
      SetWindowPos(Window, 0, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER|SWP_FRAMECHANGED);
   }
}

static bool Process_Win32_Keyboard(MSG Message)
{
   bool Result = (Message.message == WM_KEYDOWN || Message.message == WM_KEYUP ||
                  Message.message == WM_SYSKEYDOWN || Message.message == WM_SYSKEYUP);
   if(Result)
   {
      u32 Keycode = (u32)Message.wParam;
      u32 Alt_Pressed      = (Message.lParam >> 29) & 1;
      u32 Previous_State   = (Message.lParam >> 30) & 1;
      u32 Transition_State = (Message.lParam >> 29) & 1;

      bool Pressed = (Transition_State == 0);
      bool Changed = (Previous_State == Transition_State);

      switch(Keycode)
      {
         case 'F':
         case VK_F11: {
            if(Pressed && Changed)
            {
               Toggle_Win32_Fullscreen(Message.hwnd);
            }
         } break;

         case VK_RETURN: {
            if(Pressed && Changed && Alt_Pressed)
            {
               Toggle_Win32_Fullscreen(Message.hwnd);
            }
         } break;

         case VK_ESCAPE: {
            if(Pressed && Changed)
            {
               PostQuitMessage(0);
            }
         } break;
      }
   }

   return(Result);
}

LRESULT Win32_Window_Callback(HWND Window, UINT Message, WPARAM W_Param, LPARAM L_Param)
{
   LRESULT Result = 0;
   switch(Message)
   {
      case WM_CREATE: {
         if(!Is_Win32_Fullscreen(Window))
         {
            // TODO: DPI support.
            RECT Window_Rect = {0};
            Window_Rect.right = DEFAULT_RESOLUTION_WIDTH;
            Window_Rect.bottom = DEFAULT_RESOLUTION_HEIGHT;

            AdjustWindowRectEx(&Window_Rect, WS_OVERLAPPEDWINDOW, FALSE, 0);

            int Window_Width = Window_Rect.right - Window_Rect.left;
            int Window_Height = Window_Rect.bottom - Window_Rect.top;
            SetWindowPos(Window, 0, 0, 0, Window_Width, Window_Height, SWP_NOMOVE);
         }
      } break;

      case WM_PAINT: {
         PAINTSTRUCT Paint;
         HDC Device_Context = BeginPaint(Window, &Paint);

         int Width, Height;
         Get_Win32_Window_Dimensions(Window, &Width, &Height);
         PatBlt(Device_Context, 0, 0, Width, Height, BLACKNESS);

         ReleaseDC(Window, Device_Context);
      } break;

      case WM_CLOSE: {
         DestroyWindow(Window);
      } break;

      case WM_DESTROY: {
         PostQuitMessage(0);
      } break;

      default: {
         Result = DefWindowProc(Window, Message, W_Param, L_Param);
      } break;
   }

   return(Result);
}

static void Initialize_Win32(win32_context *Win32, int Show_Command)
{
   HINSTANCE Instance = GetModuleHandle(0);

   WNDCLASSEX Window_Class = {0};
   Window_Class.cbSize = sizeof(Window_Class);
   Window_Class.style = CS_HREDRAW|CS_VREDRAW;
   Window_Class.lpfnWndProc = Win32_Window_Callback;
   Window_Class.hInstance = Instance;
   Window_Class.hIcon = 0;
   Window_Class.hCursor = LoadCursor(0, IDC_ARROW);
   Window_Class.lpszClassName = TEXT("Vulkan_Win32");

   if(RegisterClassEx(&Window_Class))
   {
      DWORD Window_Style = WS_OVERLAPPEDWINDOW;
      LPCSTR Window_Name = TEXT("Vulkan Renderer (Win32)");
      Win32->Window = CreateWindowEx(0, Window_Class.lpszClassName, Window_Name, Window_Style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);

      if(Win32->Window)
      {
         ShowWindow(Win32->Window, Show_Command);
         UpdateWindow(Win32->Window);

         Win32->Running = true;
      }
   }
}

int WinMain(HINSTANCE Instance, HINSTANCE Previous_Instance, LPSTR Command_Line, int Show_Command)
{
   win32_context Win32 = {0};
   Initialize_Win32(&Win32, Show_Command);

   while(Win32.Running)
   {
      MSG Message;
      while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
      {
         if(!Process_Win32_Keyboard(Message))
         {
            if(Message.message == WM_QUIT)
            {
               Win32.Running = false;
            }
            TranslateMessage(&Message);
            DispatchMessage(&Message);
         }
      }
   }

   return(0);
}
