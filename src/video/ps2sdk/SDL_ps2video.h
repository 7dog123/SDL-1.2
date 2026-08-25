typedef struct SDL_PrivateVideoData
{
	int center_x;
	int center_y;

	float ratio;

	/* height of one interlaced field, 224 NTSC or 256 PAL */
	int screen_h;
} SDL_PrivateVideoData;
