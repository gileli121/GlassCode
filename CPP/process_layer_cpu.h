#pragma once
#include <d3d11.h>
#include "capture_layer.h"

namespace process_layer_cpu
{
	namespace map_images
	{
		void enable();
		void disable();
		bool* map_images(bool force_update_common_colors);
	}

	namespace glass_effect
	{
		void enable(const double glass_background, const bool glass_dark_background,
		            const double glass_images, const double glass_shapes);
		void disable();
		void map_shapes(double background);
		void set_background_level(const double glass_background);
		void set_shapes_level(const double glass_shapes);
		void set_dark_background_mode(const bool enable);
		
	}

	void enable_cache_buffer(bool enable);
	void init(ID3D11DeviceContext* d3d_context);
	bool load_frame(byte* pixels, int x_size, int y_size, int x_end,
	                int y_end);
	void set_default_settings();
	void free_resources();
	ID3D11Texture2D* begin_process(capture_layer::TextureData captured_texture);
	bool begin_process(ID3D11Texture2D* texture, int x_end, int y_end);
	void end_process();
	bool load_frame(byte* pixels, int x_size, int y_size, int x_end,
	                int y_end);
	void invert_colors();
	bool is_pixels_bright(byte* cpu_texture_pixels, int x_size, int y_size);
	bool is_current_pixels_bright();
	bool is_new_pixels();
}
