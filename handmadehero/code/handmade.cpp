#include "handmade.h"

internal void RenderWeirdGradient(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset) 
{
	uint8 *row = (uint8 *)Buffer->Memory;
	for (int y = 0; y < Buffer->Height; y++) 
	{
		uint32 *pixel = (uint32 *)row;

		for (int x = 0; x < Buffer->Width; x++) 
		{
			uint8 blue = x + BlueOffset;
			uint8 green = y + GreenOffset;

			*pixel++ = ((green << 8) | blue);
		}

		row += Buffer->Pitch;
	}	
}

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset)
{
	RenderWeirdGradient(Buffer, BlueOffset, GreenOffset);	
}