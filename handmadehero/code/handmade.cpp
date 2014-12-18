#include "handmade.h"

internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
    uint8 *row = (uint8 *)Buffer->Memory;
    for (int y = 0; y < Buffer->Height; y++)
    {
        uint32 *pixel = (uint32 *)row;

        for (int x = 0; x < Buffer->Width; x++)
        {
            uint8 blue = (uint8)(x + BlueOffset);
            uint8 green = (uint8)(y + GreenOffset);

            *pixel++ = ((green << 8) | blue);
        }

        row += Buffer->Pitch;
    }
}

internal void GameOutputSound(game_sound_output_buffer *SoundBuffer)
{
    local_persist real32 tSine;
    int16 ToneVolume = 3000;
    int ToneHz = 256;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16 *SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; SampleIndex++)
    {
        real32 SineValue = sinf(tSine);

        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * (real32)Pi32 * 1.0f / (real32)WavePeriod;
    }
}

internal void GameUpdateAndRender(
    game_memory *Memory,
    game_input *Input,
    game_offscreen_buffer *Buffer,
    game_sound_output_buffer *SoundBuffer
)
{
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);

    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (Memory->IsInitialized)
    {
        char *Filename = "test.bmp";

        debug_read_file_result Result = DEBUGPlatformReadEntireFile(Filename);
        if (Result.Contents)
        {
            DEBUGPlatformFreeFileMemory(Result.Contents);
        }

        GameState->ToneHz = 256;
        GameState->GreenOffset = 0;
        GameState->BlueOffset = 0;
        Memory->IsInitialized = true;
    }

    for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++)
    {
        game_controller_input *Controller = GetController(Input, ControllerIndex);

        if (Controller->IsAnalog)
        {
            // Analog
            GameState->ToneHz = 256 + (int)(128.0f * (Controller->StickAverageX));
            GameState->BlueOffset += (int)(4.0f * (Controller->StickAverageY));
        }
        else
        {
            // Digital
            if (Controller->MoveLeft.EndedDown)
            {
                GameState->BlueOffset--;
            }
            else if (Controller->MoveRight.EndedDown)
            {
                GameState->BlueOffset++;
            }
        }

        if (Controller->ActionDown.EndedDown)
        {
            GameState->GreenOffset += 1;
        }
        else if (Controller->ActionUp.EndedDown)
        {
            GameState->GreenOffset -= 1;
        }

    }

    GameOutputSound(SoundBuffer);
    RenderWeirdGradient(Buffer, GameState->BlueOffset, GameState->GreenOffset);
}