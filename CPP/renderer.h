#include <Windows.h>

namespace renderer
{

	bool init(const bool cuda_acceleration);  // Init the renderer stack memory
	void set_target(HWND target_hwnd);
	void enable_dark_mode(bool filter_images);
	void disable_dark_mode();
	enum class GlassBlurType
	{
		NONE,
		LOW,
		HIGH
	};
	bool enable_glass_mode(bool filter_images = true, GlassBlurType blur_level = GlassBlurType::LOW, double brightness_level = 1, bool dark_background = false, double background_level = 0.0, double images_level = 1.0, double texts_level = 1.0);
	void set_glass_blur_level(GlassBlurType blur_level);
	void disable_glass_mode();
	void glass_set_background_level(const double glass_background);
	void glass_set_shapes_level(const double glass_shapes);
	void glass_set_dark_background_mode(const bool enable);
	void glass_set_brightness_level(const double level);
	bool process_loop();
	bool have_fatal_error();
	void register_exit_event();
}