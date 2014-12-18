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
#include <malloc.h>
#include <stdio.h>

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

#include "win32_handmade.h"
#include "handmade.h"
#include "handmade.cpp"

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;

// Input get state
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// Input set state
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return (ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

// Sound
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

// File I/O
internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory)
{
    bool32 Result = false;

    HANDLE FileHandle = CreateFileA(
                            Filename,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            0,
                            0
                        );

    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if (WriteFile(
                    FileHandle,
                    Memory,
                    MemorySize,
                    &BytesWritten,
                    0
                ))
        {
            Result = (BytesWritten == MemorySize);
        }
        else
        {

        }

        CloseHandle(FileHandle);
    }

    return (Result);
}

internal void DEBUGPlatformFreeFileMemory(void *Memory)
{
    if (Memory)
    {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}

internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename)
{
    debug_read_file_result Result = {};

    HANDLE FileHandle = CreateFileA(
                            Filename,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            0,
                            OPEN_EXISTING,
                            0,
                            0
                        );

    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;

        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (Result.Contents)
            {
                DWORD BytesRead;
                if (ReadFile(
                            FileHandle,
                            Result.Contents,
                            FileSize32,
                            &BytesRead,
                            0
                        ) &&
                        (FileSize32 == BytesRead))
                {
                    Result.ContentsSize = FileSize32;
                }
                else
                {
                    DEBUGPlatformFreeFileMemory(Result.Contents);
                    Result.Contents = 0;
                }
            }
            else
            {

            }
        }
        else
        {

        }

        CloseHandle(FileHandle);
    }
    else
    {

    }

    return Result;
}


internal void Win32LoadXInput()
{
    HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibrary("xinput1_3.dll");
    }

    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if (!XInputGetState)
        {
            XInputGetState = XInputGetStateStub;
        }

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if (!XInputSetState)
        {
            XInputSetState = XInputSetStateStub;
        }

    }
}

internal void Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
    HMODULE DSoundLibrary = LoadLibrary("dsound.dll");

    if (DSoundLibrary)
    {
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
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER|DSBCAPS_GETCURRENTPOSITION2;

                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
                {
                    if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                    {
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
            BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;

            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
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

LRESULT CALLBACK Win32MainWindowCallback(
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
        Assert(!"Keyboard input in inappropriate place (non-dispatch message)");
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
        int height = paint.rcPaint.top - paint.rcPaint.bottom;

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

internal void Win32ClearBuffer(win32_sound_ouput *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;

    VOID *Region2;
    DWORD Region2Size;

    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(
                      0,
                      SoundOutput->SecondaryBufferSize,
                      &Region1, &Region1Size,
                      &Region2, &Region2Size,
                      0
                  )))
    {
        uint8 *DestSample = (uint8 *)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ByteIndex++)
        {
            *DestSample++ = 0;
        }

        DestSample = (uint8 *)Region2;
        for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ByteIndex++)
        {
            *DestSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void Win32FillSoundBuffer(win32_sound_ouput *SoundOutput, DWORD BytesToLock, DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer)
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
        int16 *DestSample = (int16 *)Region1;
        int16 *SourceSample = (int16 *)SourceBuffer->Samples;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; SampleIndex++)
        {
            *SampleOut++ = *SourceSample++;
            *SampleOut++ = *SourceSample++;
            SoundOutput->RunningSampleIndex++;
        }

        SampleOut = (int16 *)Region2;
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; SampleIndex++)
        {
            *SampleOut++ = *SourceSample++;
            *SampleOut++ = *SourceSample++;
            SoundOutput->RunningSampleIndex++;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown)
{
    Assert(NewState->EndedDown != IsDown);
    NewState->EndedDown = IsDown;
    NewState->HalfTransitionCount++;
}

internal void Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, DWORD ButtonBit, game_button_state *NewState)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal real32 Win32ProcessXInputStickValue(SHORT Value, SHORT Deadzone)
{
    if (Value < -Deadzone)
    {
        return (real32)Value / 32768.0f;
    }
    else if (Value > Deadzone)
    {
        return (real32)Value / 32767.0f;
    }
    return (real32)0;
}

internal void Win32ProcessPendingMessages(game_controller_input *KeyboardController)
{
    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {
        case WM_QUIT:
        {
            Running = false;
            break;
        }
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = (uint32)Message.wParam;
            bool WasDown = ((Message.lParam & (1 << 30)) != 0);
            bool IsDown = ((Message.lParam & (1 << 31)) == 0);

            if (WasDown != IsDown)
            {
                if (VKCode == 'W')
                {
                    Win32ProcessKeyboardMessage( &KeyboardController->MoveUp, IsDown);
                }
                else if (VKCode == 'A')
                {
                    Win32ProcessKeyboardMessage( &KeyboardController->MoveLeft, IsDown);
                }
                else if (VKCode == 'S')
                {
                    Win32ProcessKeyboardMessage( &KeyboardController->MoveDown, IsDown);
                }
                else if (VKCode == 'D')
                {
                    Win32ProcessKeyboardMessage( &KeyboardController->MoveRight, IsDown);
                }
                else if (VKCode == 'Q')
                {
                    Win32ProcessKeyboardMessage( &KeyboardController->LeftShoulder, IsDown);
                }
                else if (VKCode == 'E')
                {
                    Win32ProcessKeyboardMessage( &KeyboardController->RightShoulder, IsDown);
                }
                else if (VKCode == VK_UP)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
                }
                else if (VKCode == VK_LEFT)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
                }
                else if (VKCode == VK_RIGHT)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
                }
                else if (VKCode == VK_DOWN)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                }
                else if (VKCode == VK_ESCAPE)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                }
                else if (VKCode == VK_SPACE)
                {
                    Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                }
            }

            bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
            if ((VKCode == VK_F4) && AltKeyWasDown)
            {
                Running = false;
            }
            break;
        }

        default:
        {
            TranslateMessage(&Message);
            DispatchMessageA(&Message);
            break;
        }
        }
    }
}

inline LARGE_INTEGER Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline real32 Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    real32 SecondsElapsedForWork = ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency);

    return (SecondsElapsedForWork);
}

internal void Win32DebugDrawVertical(win32_offscreen_buffer *GlobalBackBuffer, int X, int Top, int Bottom, uint32 Color)
{
    uint8 *Pixel = ((uint8 *)GlobalBackBuffer->Memory + X * GlobalBackBuffer->BytesPerPixel + Top * GlobalBackBuffer->Pitch);
    for (int Y = 0; Y < Bottom; ++Y)
    {
        *(uint32 *)Pixel = Color;
        Pixel += GlobalBackBuffer->Pitch;
    }
}

internal void Win32DrawSoundBufferMarker(win32_offscreen_buffer *BackBuffer, win32_sound_ouput *SoundOutput, real32 C, int PadX, int Top, int Bottom, DWORD Value, uint32 Color)
{
    real32 XReal32 = (C * (real32)Value);

    int X = PadX + (int)XReal32;
    Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
}

internal void Win32DebugSyncDisplay(win32_offscreen_buffer *BackBuffer, int MarkerCount, win32_debug_time_marker *Markers, win32_sound_ouput *SoundOutput, real32 SecondsPerFrame)
{
    int PadX = 16;
    int PadY = 16;    

    int Top = PadY;
    int Bottom = BackBuffer->Height - PadY;

    real32 C = (real32)(BackBuffer->Width - 2 * PadX) / (real32)SoundOutput->SecondaryBufferSize;

    for (int MarkerIndex = 0; MarkerIndex < MarkerCount; MarkerIndex++)
    {
        win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];

        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->PlayCursor, 0xFFFFFFFF);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->WriteCursor, 0xFFFF0000);        
    }
}

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CommandLine,
    int ShowCode
)
{
    LARGE_INTEGER PerfCountFrequencyStruct;
    QueryPerformanceFrequency(&PerfCountFrequencyStruct);
    GlobalPerfCountFrequency = PerfCountFrequencyStruct.QuadPart;

    // NOTE: Setting the granularity of the scheduler to 1ms so that
    // we don't get weird frame misses type things.
    UINT DesiredSchedulerMilliseconds = 1;
    bool32 SleepIsGranular = timeBeginPeriod(DesiredSchedulerMilliseconds) == TIMERR_NOERROR;

    Win32LoadXInput();

    WNDCLASS WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    //  WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";
#define MonitorRefreshHz 60
#define GameUpdateHz (MonitorRefreshHz / 2)
#define FramesOfAudioLatency 2
    real32 TargetSecondsPerFrame = 1.0f / (real32)GameUpdateHz;

    if (RegisterClass(&WindowClass))
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

            HDC deviceContext = GetDC(Window);
            win32_sound_ouput SoundOutput = {};

            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.BytesPerSample = sizeof(int16) * 2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            SoundOutput.LatencySampleCount = FramesOfAudioLatency * (SoundOutput.SamplesPerSecond / GameUpdateHz);

            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);

            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);


            int16 *Samples = (int16 *)VirtualAlloc(0, 480000 * 2 * sizeof(int16), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#else
            LPVOID BaseAddress = 0;
#endif


            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes((uint64)1);

            uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);


            if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage)
            {
                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter = Win32GetWallClock();

                int DebugTimeMarkerIndex = 0;
                win32_debug_time_marker DebugTimeMarkers[GameUpdateHz / 2] = {0};

                DWORD LastPlayCursor = 0;
                bool32 SoundIsValid = false;

                int64 LastCycleCount = __rdtsc();
                while (Running)
                {
                    // TODO: Make a zeroing macro
                    game_controller_input *NewKeyboardController = GetController(NewInput, 0);
                    game_controller_input *OldKeyboardController = GetController(OldInput, 0);

                    game_controller_input ZeroController = {};
                    *NewKeyboardController = ZeroController;
                    NewKeyboardController->IsConnected = true;
                    for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ButtonIndex++)
                    {
                        NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
                    }

                    Win32ProcessPendingMessages(NewKeyboardController);

                    int MaxControllerCount = XUSER_MAX_COUNT;
                    if (MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
                    {
                        MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
                    }

                    for (DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ControllerIndex++)
                    {
                        DWORD OurControllerIndex = ControllerIndex + 1;
                        game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
                        game_controller_input *NewController = GetController(NewInput, OurControllerIndex);

                        XINPUT_STATE ControllerState;

                        if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                        {
                            NewController->IsConnected = true;

                            // Controller is plugged in
                            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                            NewController->IsAnalog = true;
                            NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

                            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                            {
                                NewController->StickAverageY = 1.0f;
                                NewController->IsAnalog = false;
                            }
                            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                            {
                                NewController->StickAverageY = -1.0f;
                                NewController->IsAnalog = false;
                            }
                            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                            {
                                NewController->StickAverageX = -1.0f;
                                NewController->IsAnalog = false;
                            }
                            if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                            {
                                NewController->StickAverageX = 1.0f;
                                NewController->IsAnalog = false;
                            }


                            // Analog stick stuff
                            real32 Threshold = 0.5f;
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageX < -Threshold) ? 1 : 0,
                                &OldController->MoveLeft, 1,
                                &NewController->MoveLeft
                            );
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageX > Threshold) ? 1 : 0,
                                &OldController->MoveRight, 1,
                                &NewController->MoveRight
                            );
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageY < -Threshold) ? 1 : 0,
                                &OldController->MoveDown, 1,
                                &NewController->MoveDown
                            );
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageY > Threshold) ? 1 : 0,
                                &OldController->MoveUp, 1,
                                &NewController->MoveUp
                            );

                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons,
                                &OldController->ActionDown, XINPUT_GAMEPAD_A,
                                &NewController->ActionDown
                            );
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons,
                                &OldController->ActionRight, XINPUT_GAMEPAD_B,
                                &NewController->ActionRight
                            );
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons,
                                &OldController->ActionLeft, XINPUT_GAMEPAD_X,
                                &NewController->ActionLeft
                            );
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons,
                                &OldController->ActionUp, XINPUT_GAMEPAD_Y,
                                &NewController->ActionUp
                            );
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons,
                                &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                &NewController->LeftShoulder
                            );
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons,
                                &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                &NewController->RightShoulder
                            );
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons,
                                &OldController->Back, XINPUT_GAMEPAD_BACK,
                                &NewController->Back
                            );
                            Win32ProcessXInputDigitalButton(
                                Pad->wButtons,
                                &OldController->Start, XINPUT_GAMEPAD_START,
                                &NewController->Start
                            );
                        }
                        else
                        {
                            NewController->IsConnected = false;
                        }
                    }


                    DWORD ByteToLock = 0;
                    DWORD TargetCursor = 0;
                    DWORD BytesToWrite = 0;
                    if (SoundIsValid)
                    {
                        ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;

                        TargetCursor = ((LastPlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize);
                        if (ByteToLock > TargetCursor)
                        {
                            BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                            BytesToWrite += TargetCursor;
                        }
                        else
                        {
                            BytesToWrite = TargetCursor - ByteToLock;
                        }
                    }

                    game_sound_output_buffer SoundBuffer = {};
                    SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                    SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                    SoundBuffer.Samples = Samples;

                    game_offscreen_buffer Buffer = {};
                    Buffer.Memory = GlobalBackBuffer.Memory;
                    Buffer.Width = GlobalBackBuffer.Width;
                    Buffer.Height = GlobalBackBuffer.Height;
                    Buffer.Pitch = GlobalBackBuffer.Pitch;

                    GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);

                    if (SoundIsValid)
                    {
                        Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
                    }

                    LARGE_INTEGER WorkCounter = Win32GetWallClock();
                    real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

                    real32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    if (SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {
                        while (SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {
                            if (SleepIsGranular)
                            {
                                DWORD SleepMilliseconds = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                                if (SleepMilliseconds > 0)
                                {
                                    Sleep(SleepMilliseconds);
                                }
                            }

                            real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());

                            while (SecondsElapsedForFrame < TargetSecondsPerFrame)
                            {
                                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                            }
                        }
                    }
                    else
                    {
                        // TODO: Missed frame rate.
                        // TODO: Logging
                    }

                    LARGE_INTEGER EndCounter = Win32GetWallClock();
                    real32 MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
                    LastCounter = EndCounter;

                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);

#if HANDMADE_INTERNAL
                    Win32DebugSyncDisplay(&GlobalBackBuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers, &SoundOutput, TargetSecondsPerFrame);
#endif
                    Win32DisplayBufferInWindow(&GlobalBackBuffer, deviceContext, Dimension.Width, Dimension.Height);

                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK)
                    {
                        LastPlayCursor = PlayCursor;
                        SoundIsValid = true;
                    }
                    else
                    {
                        SoundIsValid = false;
                    }
#if HANDMADE_INTERNAL
                    {
                        win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex++];
                        DebugTimeMarkerIndex = DebugTimeMarkerIndex % ArrayCount(DebugTimeMarkers);

                        Marker->PlayCursor = PlayCursor;
                        Marker->WriteCursor = WriteCursor;
                    }
#endif

                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;

                    uint64 EndCycleCount = __rdtsc();
                    uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    LastCycleCount = EndCycleCount;

                    real64 FPS = 0.0f;
                    real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));

                    char FPSBuffer[256];
                    _snprintf_s(FPSBuffer, sizeof(FPSBuffer),
                                "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
                    OutputDebugStringA(FPSBuffer);
                }
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

    return (0);
}