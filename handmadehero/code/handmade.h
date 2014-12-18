#if !defined(HANDMADE_H)
/*
	Services that the platform layer provides to the game
*/
#if HANDMADE_INTERNAL
	struct debug_read_file_result
	{
		uint32 ContentsSize;
		void *Contents;
	};

	internal debug_read_file_result DEBUGPlatformReadEntireFile(char *Filename);
	internal bool32 DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory);
	internal void DEBUGPlatformFreeFileMemory(void *Memory);
#endif

/*
	HANDMADE_INTERNAL:
	  0 -- Build for public
	  1 -- Release

	HANDMADE_SLOW:
	  0 -- Without assertions
	  1 -- With assertions
	
*/

// Needs timing, controller/keyboard input, bitmap, and sound buffer

#if HANDMADE_SLOW
	#define Assert(Expression) \
	    if (!(Expression)) {*(int *)0 = 0;}
#else
	#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value) * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * 1024)
#define Terabytes(Value) (Gigabytes(Value) * 1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

/*
	Services that the game provides to the platform layer
*/
inline uint32 SafeTruncateUInt64(uint64 Value)
{
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return Result;
}


struct game_offscreen_buffer
{
	void *Memory;
	int Width;
	int Height;
	int Pitch;
};

struct game_sound_output_buffer
{
	int SamplesPerSecond;
	int SampleCount;
	int16 *Samples;
};

struct game_button_state
{
	int HalfTransitionCount;
	bool32 EndedDown;
};

struct game_controller_input
{
	bool32 IsConnected;
	bool32 IsAnalog;

	real32 StickAverageX;
	real32 StickAverageY;

	union
	{
		game_button_state Buttons[12];
		struct
		{
			game_button_state MoveUp;
			game_button_state MoveDown;
			game_button_state MoveLeft;
			game_button_state MoveRight;

			game_button_state ActionUp;
			game_button_state ActionDown;
			game_button_state ActionLeft;
			game_button_state ActionRight;

			game_button_state LeftShoulder;
			game_button_state RightShoulder;

			game_button_state Back;
			game_button_state Start;
		};
	};
};

struct game_input
{
	game_controller_input Controllers[5];
};
inline game_controller_input *GetController(game_input *Input, int ControllerIndex)
{
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	
	game_controller_input *Result = &Input->Controllers[ControllerIndex];

	return Result;
}

struct game_memory
{
	bool32 IsInitialized;
	uint64 PermanentStorageSize;
	void *PermanentStorage;

	uint64 TransientStorageSize;
	void *TransientStorage;
};

struct game_state
{
	int ToneHz;
	int GreenOffset;
	int BlueOffset;
};

internal void GameUpdateAndRender(game_input *Input, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer);

#define HANDMADE_H
#endif