/* (c) copyright 2025 Lawrence D. Kern /////////////////////////////////////// */

// NOTE: This file is the entry point for the Win32-based Windows build. Each
// supported platform will have its code constrained to a single file, which
// will quarantine the underlying platform from the renderer.

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

#include <stdio.h>

#include "shared.h"
#include "platform.h"
#include "vulkan_renderer.h"

typedef struct {
   HINSTANCE Instance;
   HWND Window;

   LARGE_INTEGER Frequency;
   LARGE_INTEGER Frame_Start;
   LARGE_INTEGER Frame_End;
   u64 Frame_Count;

   bool Running;
   bool Rendering_Paused;

   vulkan_context VK;
} win32_context;

#include "vulkan_renderer.c"

static LOG(Log)
{
   static char Message[512];

   va_list Arguments;
   va_start(Arguments, Format);
   vsnprintf(Message, sizeof(Message), Format, Arguments);
   va_end(Arguments);

   OutputDebugString(Message);
}

static READ_ENTIRE_FILE(Read_Entire_File)
{
   string Result = {0};

   WIN32_FIND_DATAA File_Data;
   HANDLE Find_File = FindFirstFileA(Path, &File_Data);
   if(Find_File == INVALID_HANDLE_VALUE)
   {
      Log("Failed to find file \"%s\".\n", Path);
   }
   else
   {
      FindClose(Find_File);

      size Length = (File_Data.nFileSizeHigh * (MAXDWORD + 1)) + File_Data.nFileSizeLow;
      Result.Data = VirtualAlloc(0, Length+1, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
      if(!Result.Data)
      {
         Log("Failed to allocate memory for file \"%s\".\n", Path);
      }
      else
      {
         // NOTE(law): ReadFile is limited to reading 32-bit file sizes. As a
         // result, the Win32 platform can't actually use the full 64-bit size
         // file size defined in the non-platform code.

         HANDLE File = CreateFileA(Path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
         DWORD Bytes_Read;
         if(ReadFile(File, Result.Data, (DWORD)Length, &Bytes_Read, 0) && Length == (size)Bytes_Read)
         {
            Result.Length = Length;
            Result.Data[Length] = 0;
         }
         else
         {
            Log("Failed to read file \"%s.\"\n", Path);

            VirtualFree(Result.Data, 0, MEM_RELEASE);
            ZeroMemory(Result.Data, Length+1);
         }
         CloseHandle(File);
      }
   }

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

      win32_context *Win32 = (win32_context *)GetWindowLongPtr(Message.hwnd, GWLP_USERDATA);

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
               Win32->Running = false;
            }
         } break;
      }
   }

   return(Result);
}

LRESULT Win32_Window_Callback(HWND Window, UINT Message, WPARAM W_Param, LPARAM L_Param)
{
   LRESULT Result = 0;

   win32_context *Win32 = (win32_context *)GetWindowLongPtr(Window, GWLP_USERDATA);
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

      case WM_SIZE: {
         if(Win32)
         {
            if(W_Param == SIZE_MINIMIZED)
            {
               Win32->Rendering_Paused = true;
            }
            else
            {
               Win32->Rendering_Paused = false;
               Win32->VK.Resize_Requested = true;
            }
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

      case WM_QUIT:
      case WM_DESTROY: {
         if(Win32)
         {
            Win32->Running = false;
         }
      } break;

      default: {
         Result = DefWindowProc(Window, Message, W_Param, L_Param);
      } break;
   }

   return(Result);
}

static float Compute_Win32_Frame_Time(win32_context *Win32)
{
   float Frame_Seconds_Elapsed = 1.0f / 60.0f;

   QueryPerformanceCounter(&Win32->Frame_End);
   if(Win32->Frame_Start.QuadPart)
   {
      Frame_Seconds_Elapsed = ((float)(Win32->Frame_End.QuadPart - Win32->Frame_Start.QuadPart) / (float)Win32->Frequency.QuadPart);
   }

#if DEBUG
   if((Win32->Frame_Count++ % 60) == 0)
   {
      Log("Frame Time: %fms \r", Frame_Seconds_Elapsed * 1000.0f);
   }
#endif

   Win32->Frame_Start = Win32->Frame_End;
   Win32->Frame_Count++;

   return(Frame_Seconds_Elapsed);
}

static void Initialize_Win32(win32_context *Win32, int Show_Command)
{
   Win32->Instance = GetModuleHandle(0);
   QueryPerformanceFrequency(&Win32->Frequency);

   WNDCLASSEX Window_Class = {0};
   Window_Class.cbSize = sizeof(Window_Class);
   Window_Class.style = CS_HREDRAW|CS_VREDRAW;
   Window_Class.lpfnWndProc = Win32_Window_Callback;
   Window_Class.hInstance = Win32->Instance;
   Window_Class.hIcon = 0;
   Window_Class.hCursor = LoadCursor(0, IDC_ARROW);
   Window_Class.lpszClassName = TEXT("Vulkan_Win32");

   if(RegisterClassEx(&Window_Class))
   {
      DWORD Window_Style = WS_OVERLAPPEDWINDOW;
      LPCSTR Window_Name = TEXT("Vulkan Renderer (Win32)");
      Win32->Window = CreateWindowEx(0, Window_Class.lpszClassName, Window_Name, Window_Style, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Win32->Instance, 0);

      if(Win32->Window)
      {
         SetWindowLongPtr(Win32->Window, GWLP_USERDATA, (LONG_PTR)Win32);

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

   if(Initialize_Vulkan(&Win32.VK, &Win32))
   {
      while(Win32.Running)
      {
         MSG Message;
         while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
         {
            if(!Process_Win32_Keyboard(Message))
            {
               TranslateMessage(&Message);
               DispatchMessage(&Message);
            }
         }

         float Frame_Seconds_Elapsed = Compute_Win32_Frame_Time(&Win32);
         if(!Win32.Rendering_Paused)
         {
            Render_With_Vulkan(&Win32.VK, Frame_Seconds_Elapsed);
         }
      }

      Destroy_Vulkan(&Win32.VK);
   }

   return(0);
}
