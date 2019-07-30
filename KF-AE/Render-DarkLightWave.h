#pragma once
#include "render.h"

class Render_DarkLightWave {
	public:
	static PF_Err Render8(void *refcon, A_long x, A_long y, PF_Pixel8 *in, PF_Pixel8 *out);
	static PF_Err Render16(void *refcon, A_long x, A_long y, PF_Pixel16 *in, PF_Pixel16 *out);
	static PF_Err Render32(void *refcon, A_long x, A_long y, PF_Pixel32 *in, PF_Pixel32 *out);
};
