#pragma once
#include <d3d11.h>


namespace process_layer_gpu
{
	void init(ID3D11DeviceContext* d3d_context);
	bool enable_cache_buffer(const bool enable);
	bool init_frame(int x_size, int y_size, int x_end = 0, int y_end = 0);
	void set_default_settings();
	void free_resources();

	namespace map_images
	{
		void enable();
		void disable();
	}

	namespace glass_effect
	{
		void enable(double glass_background, bool glass_dark_background, double glass_images,
		            double glass_shapes);
		void disable();
		bool map_shapes();
		void set_background_level(const double glass_background);
		void set_shapes_level(const double glass_shapes);
		void set_dark_background(const bool enable);
	}

	bool begin_process(ID3D11Texture2D* processed_texture, int capture_x_size, int capture_y_size);

	bool is_new_pixels(bool& error);
	bool end_process();
	bool set_image_area_data(bool* image_area_data);
	bool is_current_pixels_bright(bool& error);

	bool invert_colors();
}
