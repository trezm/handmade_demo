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
#include <dsound.h>
#include <math.h>

#define internal static
#define local_persist static
#define global_variable static
#define Pi32 3.14159265359

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;
typedef float real32;
typedef double real64;

#include "handmade.h"
#include "handmade.cpp"

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
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
// Input get state
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// Input set state
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// Sound
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32LoadXInput()
{
	HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
	if (!XInputLibrary) {
		XInputLibrary = LoadLibrary("xinput1_3.dll");
	}

	if (XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		if (!XInputGetState) {
			XInputGetState = XInputGetStateStub;
		}

		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
		if (!XInputSetState) {
			XInputSetState = XInputSetStateStub;
		}

	}
}

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize) 
{
	HMODULE DSoundLibrary = LoadLibrary("dsound.dll");

	if (DSoundLibrary) {
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) 
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;			
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign; 
			WaveFormat.cbSize = 0;

			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) 
				{
					if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat))) {
						OutputDebugStringA("Primary Buffer format was set.\n");
					}
					else
					{

					}
				}
				else
				{

				}
			}
			else
			{

			}

			// Create secondary buffer
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0))) 
			{
			}

			// Other stuff
		}
		else
		{

		}
	}
	else
	{

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
			uint32 VKCode = WParam;
			bool WasDown = ((LParam & (1 << 30)) != 0);
			bool IsDown = ((LParam & (1 << 31)) == 0);

			if (WasDown != IsDown) {
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
			}

			bool AltKeyWasDown = ((LParam & (1 << 29)) != 0);
			if ((VKCode == VK_F4) && AltKeyWasDown) 
			{
				Running = false;
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

struct win32_sound_ouput
{
	int SamplesPerSecond;
	int ToneHz;
	int ToneVolume;
	uint32 RunningSampleIndex;
	int WavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
};

internal void Win32FillSoundBuffer(win32_sound_ouput *SoundOutput, DWORD BytesToLock, DWORD BytesToWrite) 
{
	VOID *Region1;
	DWORD Region1Size;

	VOID *Region2;
	DWORD Region2Size;

	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
		BytesToLock,
		BytesToWrite,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		0
		)))
	{
		int16 *SampleOut = (int16 *)Region1;
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++) 
		{
			real32 t = ((real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod) * 2.0f * Pi32;
			real32 SineValue = sinf(t);

			int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;
			SoundOutput->RunningSampleIndex++;
		}

		SampleOut = (int16 *)Region2;
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
		{
			real32 t = ((real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod) * 2.0f * Pi32;
			real32 SineValue = sinf(t);

			int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;
			SoundOutput->RunningSampleIndex++;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);					
	}
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
			// Graphics Test
			int xOffset = 0;
			int yOffset = 0;

			win32_sound_ouput SoundOutput = {};

			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.ToneHz = 256;
			SoundOutput.ToneVolume = 6000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond / SoundOutput.ToneHz;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);

			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);

			LARGE_INTEGER PerfCountFrequencyStruct;
			QueryPerformanceFrequency(&PerfCountFrequencyStruct);
			int64 PerfCountFrequency = PerfCountFrequencyStruct.QuadPart;

			int64 LastCycleCount = __rdtsc();
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

						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;

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

				game_offscreen_buffer Buffer = {};
				Buffer.Memory = GlobalBackBuffer.Memory;
				Buffer.Width = GlobalBackBuffer.Width;
				Buffer.Height = GlobalBackBuffer.Height;
				Buffer.Pitch = GlobalBackBuffer.Pitch;

				GameUpdateAndRender(&Buffer, xOffset, yOffset);

				// Direct sound output test
				DWORD PlayCursor;
				DWORD WriteCursor;
				if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
					DWORD BytesToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
					DWORD BytesToWrite;
					if (BytesToLock == PlayCursor)
					{
						BytesToWrite = 0;
					}
					else if (BytesToLock > PlayCursor) 
					{
						BytesToWrite = (SoundOutput.SecondaryBufferSize - BytesToLock);
						BytesToWrite += PlayCursor;
					}
					else
					{
						BytesToWrite = PlayCursor - BytesToLock;
					}

					Win32FillSoundBuffer(&SoundOutput, BytesToLock, BytesToWrite);
				}

				HDC deviceContext = GetDC(Window);

				win32_window_dimension Dimension = Win32GetWindowDimension(Window);			
				Win32DisplayBufferInWindow(&GlobalBackBuffer, deviceContext, Dimension.Width, Dimension.Height);

				ReleaseDC(Window, deviceContext);

				xOffset++;

				int64 EndCycleCount = __rdtsc();

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);
				int64 CyclesElapsed = EndCycleCount - LastCycleCount;
				int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				int64 MSPerFrame = ((1000 * CounterElapsed) / PerfCountFrequency);
				int64 FPS = PerfCountFrequency / CounterElapsed;
				int64 MCPF = (int32)(CyclesElapsed / (1000 * 1000));

/*
				char Buffer[256];
				wsprintf(Buffer, "%dms/f, %dFPS, %dmc/f\n", MSPerFrame, FPS, MCPF);
				OutputDebugStringA(Buffer);
*/

				LastCycleCount = EndCycleCount;
				LastCounter = EndCounter;
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