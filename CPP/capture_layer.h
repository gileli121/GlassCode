#pragma once

namespace capture_layer
{
	struct TextureData
	{
		ID3D11Texture2D* textrue = nullptr;
		int x_size = -1;
		int y_size = -1;
	};


	// Textrue data that we got from callback


	inline bool capturing = false;

	bool init();
	void set_target(HWND target_hwnd);
	bool create_layer();
	void start_capture_session();
	void dispose();
	bool get_new_frame(TextureData* texture_data);
}
