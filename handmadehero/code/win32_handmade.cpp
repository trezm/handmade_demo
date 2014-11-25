/*
	$File: $
	$Date: $
	$Revision: $
	$Creator: Pete Mertz $
	$Notice: (C) Pete Mertz 2014 $
*/

#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define internal static
#define local_persist static
#define global_variable static

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void *Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension
{
	int Width;
	int Height;
};

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

// Input get state
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return(0);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;

// Input set state
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(0);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

internal void Win32LoadXInput()
{
	HMODULE XInputLibrary = LoadLibrary("xinput1_3.dll");
	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

internal win32_window_dimension Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT clientRect;
	GetClientRect(Window, &clientRect);
	Result.Width = clientRect.right - clientRect.left;
	Result.Height = clientRect.bottom - clientRect.top;

	return Result;
}

internal void renderWeirdGradient(win32_offscreen_buffer Buffer, int blueOffset, int greenOffset)
{
	uint8_t *row = (uint8_t *)Buffer.Memory;
	for (int y = 0; y < Buffer.Height; y++) 
	{
		uint32_t *pixel = (uint32_t *)row;

		for (int x = 0; x < Buffer.Width; x++) 
		{

			/*
	            Memory:   BB GG RR xx
	            Register: xx RR GG BB
			*/

			uint8_t blue = x + blueOffset;
			uint8_t green = y + greenOffset;

			*pixel++ = ((green << 8) | blue);
		}

		row += Buffer.Pitch;
	}

}

internal void Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) 
{
	if (Buffer->Memory) 
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}

	Buffer->Width = Width;
	Buffer->Height = Height;
	Buffer->BytesPerPixel = 4;


	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int bitmapMemorySize = Buffer->BytesPerPixel * (Buffer->Width * Buffer->Height);
	Buffer->Memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC deviceContext, 
	int Width, int Height)
{


	StretchDIBits(
		deviceContext,
		0, 0, Width, Height,		
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory,
		&Buffer->Info,
		DIB_RGB_COLORS,
		SRCCOPY
		);
}

LRESULT CALLBACK MainWindowCallback(
  HWND Window,
  UINT Message,
  WPARAM WParam,
  LPARAM LParam
)
{
	LRESULT result = 0;

	switch (Message) 
	{
		case WM_SIZE: 
		{
			break;
		}

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32_t VKCode = WParam;
			bool WasDown = ((LParam & (1 << 30)) != 0);
			bool IsDown = ((LParam & (1 << 31)) == 0);

			if (VKCode == 'W')
			{

			}
			else if (VKCode == 'A')
			{
				
			}
			else if (VKCode == 'S')
			{
				
			}
			else if (VKCode == 'D')
			{
				
			}
			else if (VKCode == 'Q')
			{
				
			}
			else if (VKCode == 'E')
			{
				
			}
			else if (VKCode == VK_UP)
			{
				
			}
			else if (VKCode == VK_LEFT)
			{
				
			}
			else if (VKCode == VK_RIGHT)
			{
				
			}
			else if (VKCode == VK_DOWN)
			{
				
			}
			else if (VKCode == VK_ESCAPE)
			{
				
			}
			else if (VKCode == VK_SPACE)
			{
				
			}
			break;
		}

		case WM_DESTROY: 
		{
			Running = false;
			break;
		}

		case WM_CLOSE: 
		{
			Running = false;
			break;
		}

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
			break;
		}

		case WM_PAINT:
		{
			PAINTSTRUCT paint;

			HDC deviceContext = BeginPaint(Window, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int width = paint.rcPaint.right - paint.rcPaint.left;
			int	height = paint.rcPaint.top - paint.rcPaint.bottom;

			win32_window_dimension Dimension = Win32GetWindowDimension(Window);			
			Win32DisplayBufferInWindow(&GlobalBackBuffer, deviceContext, Dimension.Width, Dimension.Height);
			EndPaint(Window, &paint);
			break;
		}

		default:
		{
			result = DefWindowProc(Window, Message, WParam, LParam);
			break;
		}
	}

	return result;
}

int CALLBACK WinMain(
  HINSTANCE Instance,
  HINSTANCE PrevInstance,
  LPSTR CommandLine,
  int ShowCode
)
{
	Win32LoadXInput();

	WNDCLASS WindowClass = {};

	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = MainWindowCallback;
	WindowClass.hInstance = Instance;
//	WindowClass.hIcon;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

	if(RegisterClass(&WindowClass)) 
	{
		HWND Window = CreateWindowEx(
			0,
			WindowClass.lpszClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW | WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance,
			0
			);
		if (Window) 
		{
			Running = true;
			int xOffset = 0;
			int yOffset = 0;
			while(Running)
			{
				MSG message;
				while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)) 
				{
					if (message.message == WM_QUIT) 
					{
						Running = false;
					}

					TranslateMessage(&message);
					DispatchMessageA(&message);
				}

				for (DWORD i = 0; i < XUSER_MAX_COUNT; i++)
				{
					XINPUT_STATE ControllerState;

					if (XInputGetState(i, &ControllerState) == ERROR_SUCCESS) {
						// Controller is plugged in
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);						
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);						
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);												
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);																								bool DPadUp = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);												
						bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);												
						bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);												
						bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);												
						bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);																																				
						bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);												

						int16_t StickX = Pad->sThumbLX;
						int16_t StickY = Pad->sThumbLY;

						if (AButton)
						{
							yOffset += 2;
						}
					}
					else
					{
						// Controller is unplugged
					}
				}

				renderWeirdGradient(GlobalBackBuffer, xOffset, yOffset);

				HDC deviceContext = GetDC(Window);

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);			
				Win32DisplayBufferInWindow(&GlobalBackBuffer, deviceContext, Dimension.Width, Dimension.Height);

				ReleaseDC(Window, deviceContext);

				xOffset++;
			}
		}
		else
		{

		}

	}
	else
	{

	}

	return(0);
}