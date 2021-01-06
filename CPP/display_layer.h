#include <Windows.h>

namespace display_layer
{
	struct Buffer
	{
		byte* pixels = nullptr;
		int x_size = 0;
		int y_size = 0;
		unsigned buffer_size = 0;
	};

	inline Buffer buffer = {0};
	inline RECT target_rect = {0};

	bool init();
	void dispose();
	void set_target(HWND target_hwnd);
	bool create_layer();
	void dispose();

	enum class BlurType
	{
		NONE,
		LOW,
		HIGH
	};

	void set_blur_type(BlurType blur_type);
	void set_brightness_level(int level);
	bool create_screen_buffer();
	void draw_texture(ID3D11Texture2D* texture);
	bool update_target_rect();
	void move_layer_to_target();
	void hide_target_hwnd();
	void show_target_hwnd();
	void hide_layer();
	void show_layer();
	void set_layer_to_foreground();
	bool set_target_window_transparent();
	void un_set_target_window_transparent();
}
