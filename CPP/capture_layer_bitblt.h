#pragma once
#include <Windows.h>

namespace capture_layer_bitblt
{
	inline byte* pixels = nullptr;
	bool capture(int x_pos, int y_pos, int x_size, int y_size);
	void un_init_capture();
}
