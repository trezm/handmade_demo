#if !defined(HANDMADE_H)
/*
	Services that the game provides to the platform layer
*/

/*
	Services that the platform layer provides to the game
*/

// Needs timing, controller/keyboard input, bitmap, and sound buffer

struct game_offscreen_buffer
{
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset);

#define HANDMADE_H
#endif