#include <dwmapi.h>
#include <psapi.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <Windows.h>
#include <d3d11.h>
#pragma comment(lib, "D3D11.lib")
#include "cuda_runtime.h"
#include "device_launch_parameters.h"
#pragma comment(lib, "Dwmapi.lib")
#include <plog/Log.h>


#include "capture_layer.h"
#include "capture_layer_bitblt.h"
#include "display_layer.h"
#include "graphic_device.h"
#include "process_layer_cpu.h"
#include "process_layer_gpu.h"

#include "renderer.h"

#include <iostream>

namespace renderer
{
	/**
	 * \brief Indicates if we currently re rendering the window or not
	 */
	bool rendering = false;

	/**
	 * \brief The handle of the window to re render
	 */
	HWND target_hwnd = nullptr;

	/**
	 * \brief The current WINDOWPLACEMENT structure of the window
	 * (used inside process_window_placement function)
	 */
	WINDOWPLACEMENT target_placement = {0};

	/**
	 * \brief x_size and y_size of the frame that captured from the window
	 */
	int x_size = 0, y_size = 0;

	/**
	 * \brief ID3D11Texture2D resource that used inside process_frame_in_cpu
	 * for processing
	 */
	ID3D11Texture2D* cpu_texture = nullptr;

	/**
	 * \brief ID3D11Texture2D resource that used inside process_frame_in_gpu
	 * for processing
	 */
	ID3D11Texture2D* gpu_texture = nullptr;

	/**
	 * \brief Indicates if dark mode is enabled
	 */
	bool dark_mode = false;

	/**
	 * \brief Indicates if glass mode is enabled
	 */
	bool glass_mode = false;

	/**
	 * \brief Indicates the type of blur effect of the glass mode
	 */
	GlassBlurType glass_blur_type;

	/**
	 * \brief Other settings of the glass effect
	 */
	double glass_brightness_level, glass_background, glass_images, glass_texts;
	bool glass_dark_background;

	/**
	 * \brief Handle of the thread that runs function process_frame_thread
	 */
	std::thread process_frame_thread_handle;

	/**
	 * \brief Indicates if the target window is in use
	 */
	bool is_target_window_in_use = false;

	/**
	 * \brief Flag to signal to the process frame thread to stop in case it is true
	 */
	auto run_process_frame_thread = false;

	/**
	 * \brief The process frame thread is setting this flag to true before exiting
	 * to signal to the main thread if the process thread still doing some job or not
	 */
	bool process_frame_thread_exited = false;

	/**
	 * \brief Indicates if there is some fatal error or not
	 */
	bool fatal_error = false;

	/**
	 * \brief Indicates if there is fatal error in the process frame thread
	 */
	bool frame_thread_fatal_error = false;

	/**
	 * \brief Used inside process_frame_thread function (internal usage)
	 */
	clock_t resize_timer = 0;
	bool window_temporary_hidden = false;


	/**
	 * \brief Used to know when to check if the window frame is bright
	 * it will update the flag pixels_bright each time
	 */
	clock_t brightness_check_timer = 0;
	constexpr int brightness_check_timer_interval = 1000;

	/**
	 * \brief Indicates if the current frame of the window is bright
	 */
	bool pixels_bright = true;


	/**
	 * \brief Indicates if the window is currently hidden (minimized or not shown)
	 */
	bool window_hidden = false;

	/**
	 * \brief When the function init_for_target_hwnd will be called it will check
	 * this flag. if it set to true, it will do more initialization actions
	 * that should done when the user just enabled the rendering effect
	 * (such as dark mode/glass mode)
	 */
	bool startup_rendering = false;


	/**
	 * \brief When this variable is set to non 0, it will signal the process_frame_thread
	 * to call to process_frame_in_gpu or process_frame_in_cpu functions with
	 * flag force_render = true for a given amount of time that defined in the
	 * process_frame_thread function
	 */
	clock_t force_render_timer = 0;

	/**
	 * \brief While the window is maximized we use this timer to wait a bit before recreating/resizing
	 * the frame
	 */
	clock_t was_maximized_timer = 0;

	/**
	 * \brief While the window is minimized we use this timer to wait a little before stopping re-rendering
	 * the window. this is to avoid bug with intellij that when minimizing the window become blank
	 */
	clock_t was_minimized_timer = 0;

	/**
	 * \brief Indicates if functions process_frame_in_gpu or process_frame_in_cpu should
	 * apply filter image algorithms that implemented in process_layer_cpu or process_layer_gpu
	 */
	bool filter_images = false;

	/**
	 * \brief If this flag is true, it will signal to the process_frame_thread function
	 * to wait a little before start processing the captured frames of the window
	 */
	bool start_processing_wait = false;


	/**
	 * \brief This flag is used by register_exit_event() to exit and dispose the renderer in thread-safe way
	 */
	bool exit_event_requested = false;


	// Forward Declarations
	void process_frame_thread();
	void start_process_frame_thread();
	void stop_process_frame_thread();

	/**
	 * \brief Indicates if there is fatal error
	 * \return true in case of fatal error, false otherwise
	 */
	bool have_fatal_error()
	{
		return fatal_error;
	}

	/**
	 * \brief Register request to exit and dispose the renderer in the future
	 * in thread-safe way
	 */
	void register_exit_event()
	{
		exit_event_requested = true;
	}

	/**
	 * \brief Init function that you should call at startup only.
	 * It setup GPU acceleration if needed and other settings that never change during the
	 * lifetime of the program.
	 * \param cuda_acceleration
	 * \return true if successful and false on failure
	 */
	bool init(const bool cuda_acceleration)
	{
		std::cout << "Initializing renderer\n";

		fatal_error = false;

		if (!graphic_device::init_device(cuda_acceleration))
		{
			std::cout << "Failed to load_frame graphic device for rendering\n";
			return false;
		}

		process_layer_cpu::init(graphic_device::d3d_context);

		if (graphic_device::is_cuda_adapter)
		{
			process_layer_gpu::init(graphic_device::d3d_context); // TODO: Maybe remove this...
		}

		if (!display_layer::init())
		{
			std::cout << "Failed to load_frame display_layer\n";
			return false;
		}

		if (!capture_layer::init())
		{
			std::cout << "Failed to load_frame capture_layer\n";
			return false;
		}

		return true;
	}

	/**
	 * \brief Set on the window the type of transparency-blur effect
	 * \param blur_level
	 */
	void set_glass_blur_level(const GlassBlurType blur_level)
	{
		switch (blur_level)
		{
		case GlassBlurType::LOW:
			set_blur_type(display_layer::BlurType::LOW);
			break;
		case GlassBlurType::HIGH:
			set_blur_type(display_layer::BlurType::HIGH);
			break;
		case GlassBlurType::NONE:
		default:
			set_blur_type(display_layer::BlurType::NONE);
			break;
		}

		glass_blur_type = blur_level;
	}

	void glass_set_background_level(const double glass_background)
	{
		if (graphic_device::is_cuda_adapter)
			process_layer_gpu::glass_effect::set_background_level(glass_background);
		else
			process_layer_cpu::glass_effect::set_background_level(glass_background);
	}

	void glass_set_shapes_level(const double glass_shapes)
	{
		if (graphic_device::is_cuda_adapter)
			process_layer_gpu::glass_effect::set_shapes_level(glass_shapes);
		else
			process_layer_cpu::glass_effect::set_shapes_level(glass_shapes);
	}

	void glass_set_dark_background_mode(const bool enable)
	{
		if (graphic_device::is_cuda_adapter)
			process_layer_gpu::glass_effect::set_dark_background(enable);
		else
			process_layer_cpu::glass_effect::set_background_level(enable);
	}

	void glass_set_brightness_level(const double level)
	{
		display_layer::set_brightness_level(level * 255);
	}

	/**
	 * \brief Set the target window for re rendering
	 * \param target_hwnd - The handle of the window to set as target for re rendering
	 */
	void set_target(const HWND target_hwnd)
	{
		fatal_error = false;
		window_temporary_hidden = false;
		renderer::target_hwnd = target_hwnd;
		display_layer::set_target(target_hwnd);
		capture_layer::set_target(target_hwnd);
	}

	/**
	 * \brief After the target window has been set, this function used to startup
	 * the actual re rendering process
	 * \param wait - Should we wait for few seconds after capture started or not
	 * \return true on success and false of failure
	 */
	bool init_for_target_hwnd(const bool wait = false)
	{
		if (rendering) return true;

		std::cout << "Init renderer target window\n";

		if (!GetWindowPlacement(target_hwnd, &target_placement))
		{
			std::stringstream ss;
			ss << "Failed to get window placement, Error: " << GetLastError() << std::endl;;
			std::cout << ss.str();
			return false;
		}

		// Create the display layer
		if (!display_layer::create_layer())
		{
			std::cout << "Failed to create capture layer\n";
			return false;
		}


		if (glass_mode)
		{
			if (!display_layer::set_target_window_transparent())
			{
				std::cout << "Failed to set target window transparent\n";
				display_layer::dispose();
				return false;
			}

			set_glass_blur_level(glass_blur_type);
			if (glass_brightness_level < 1.0)
				display_layer::set_brightness_level(glass_brightness_level * 255);
		}


		// TODO: Maybe I should remove it
		if (!display_layer::create_screen_buffer())
		{
			std::cout << "Failed to create screen buffer for display layer\n";
			return false;
		}

		// Need to do it (ugly workaround) to avoid bug in WinRT capture API
		display_layer::target_rect.top -= 20;
		display_layer::target_rect.left -= 20;
		display_layer::move_layer_to_target();
		if (startup_rendering)
		{
			display_layer::set_layer_to_foreground();
			startup_rendering = false;
		}


		// Create the capture layer
		if (!capture_layer::create_layer())
		{
			std::cout << "Failed to create capture layer\n";
			return false;
		}


		process_layer_cpu::free_resources();
		process_layer_cpu::set_default_settings();

		if (graphic_device::is_cuda_adapter)
		{
			process_layer_gpu::free_resources();
			process_layer_gpu::set_default_settings();
		}


		if (filter_images)
		{
			process_layer_cpu::map_images::enable();
			process_layer_cpu::enable_cache_buffer(true);
		}
		else
		{
			if (graphic_device::is_cuda_adapter)
			{
				process_layer_cpu::enable_cache_buffer(false);
				if (!process_layer_gpu::enable_cache_buffer(true))
				{
					std::cout << "process_layer_gpu::enable_cache_buffer(true) failed\n";
					return false;
				}
			}
			else
			{
				process_layer_gpu::enable_cache_buffer(false);
				process_layer_cpu::enable_cache_buffer(true);
			}
		}

		if (graphic_device::is_cuda_adapter)
		{
			if (glass_mode)
				process_layer_gpu::glass_effect::enable(glass_background, glass_dark_background, glass_images,
				                                        glass_texts);
			else
				process_layer_gpu::glass_effect::disable();
		}
		else
		{
			if (glass_mode)
				process_layer_cpu::glass_effect::enable(glass_background, glass_dark_background, glass_images,
				                                        glass_texts);
			else
				process_layer_cpu::glass_effect::disable();
		}


		capture_layer::start_capture_session();

		force_render_timer = clock();
		x_size = y_size = 0;
		start_processing_wait = wait;
		start_process_frame_thread();


		is_target_window_in_use = true;
		rendering = true;
		return true;
	}

	/**
	 * \brief Shutdown the re rendering of the target window that defined in set_target
	 */
	void un_init_for_target_hwnd()
	{
		// Shutdown the frames thread if needed
		stop_process_frame_thread();

		if (glass_mode)
			display_layer::un_set_target_window_transparent();
		display_layer::dispose();
		capture_layer::dispose();
		capture_layer_bitblt::un_init_capture();
		process_layer_cpu::free_resources();
		process_layer_gpu::free_resources();
		x_size = y_size = 0;
		rendering = false;
		is_target_window_in_use = false;
	}

	/**
	 * \brief Function is used to start the thread that used to process new frames
	 * from the window
	 */
	void start_process_frame_thread()
	{
		run_process_frame_thread = true;
		process_frame_thread_handle = std::thread(process_frame_thread);
		process_frame_thread_handle.detach();
		const auto timer = clock();
		while (process_frame_thread_exited && clock() - timer < 2000);

		EmptyWorkingSet(GetCurrentProcess()); // Reduce memory usage
	}

	/**
	 * \brief Function is used to stop the thread that used to process new frames
	 */
	void stop_process_frame_thread()
	{
		if (run_process_frame_thread && !process_frame_thread_exited)
		{
			const auto timer = clock();

			// Send signal to the thread to close itself and wait until it closed
			run_process_frame_thread = false;
			while (!process_frame_thread_exited && clock() - timer <= 2000);
		}
	}

	/**
	 * \brief This function is used when the size of the captured frame was changed
	 * or it is the first frame and no frame was processed before.
	 * This function is responsible to allocate memory and other resources
	 * that used while processing the captured frame in CPU only
	 * \param captured_texture - The texture (captured frame) of the window
	 * \return true on success, false on failure
	 */
	bool init_cpu_process_mode(ID3D11Texture2D* captured_texture)
	{
		if (cpu_texture)
		{
			cpu_texture->Release();
			cpu_texture = nullptr;
		}

		if (!graphic_device::create_texture(&cpu_texture, D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,
		                                    D3D11_USAGE_STAGING))
		{
			std::cout << "Failed to init cpu_texture\n";
			return false;
		}

		graphic_device::copy_texture(cpu_texture, captured_texture);

		return cpu_texture;
	}

	/**
	 * \brief This function is used when the size of the captured frame was changed
	 * or it is the first frame and no frame was processed before.
	 * This function is responsible to allocate memory and other resources
	 * that used while processing the captured frame in GPU in addition to CPU.
	 * It may also decide to allocate memory and resources only for the GPU so
	 * the all processing will be done in the GPU
	 * \param captured_texture - The texture (captured frame) of the window
	 * \return true on success, false on failure
	 */
	bool init_gpu_process_mode(ID3D11Texture2D* captured_texture)
	{
		if (!init_cpu_process_mode(captured_texture))
		{
			std::cout << "init_cpu_process_mode(*) failed\n";
			return false;
		}

		if (gpu_texture)
		{
			gpu_texture->Release();
			gpu_texture = nullptr;
		}

		if (!graphic_device::create_texture(&gpu_texture, D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,
		                                    D3D11_USAGE_STAGING))
		{
			std::cout << "Failed to init gpu_texture\n";
			if (cpu_texture)
			{
				cpu_texture->Release();
				cpu_texture = nullptr;
			}
			return false;
		}

		graphic_device::copy_texture(gpu_texture, captured_texture);

		return true;
	}


	/**
	 * \brief This function will process any given frame in the GPU and maybe
	 * also in CPU if there is no other option to process everything in GPU.
	 * This function will fail if you never called to init_gpu_process_mode
	 * \param captured_texture - The given frame to process
	 * \param force_render - Use this flag to force reprocessing even if the frame
	 * is the exact frame as before
	 * \param new_frame - (OUT) This is output parameter that indicates if there
	 * was a new frame
	 * \return true in case no errors occurred, false in case there is error
	 */
	bool process_frame_in_gpu(ID3D11Texture2D* captured_texture, const bool force_render, bool& new_frame)
	{
		bool* image_area_data = nullptr;

		if (filter_images)
		{
			graphic_device::copy_texture(cpu_texture, captured_texture);

			if (!process_layer_cpu::begin_process(cpu_texture, x_size, y_size))
			{
				std::cout << "process_layer_cpu::begin_process(*) failed\n";
				return false; // Signal fatal error
			}

			new_frame = force_render || process_layer_cpu::is_new_pixels();

			if (!new_frame)
			{
				process_layer_cpu::end_process();
				return true;
			}


			image_area_data = process_layer_cpu::map_images::map_images(force_render);
			process_layer_cpu::end_process();

			graphic_device::copy_texture(gpu_texture, cpu_texture);
		}
		else
		{
			graphic_device::copy_texture(gpu_texture, captured_texture);
		}


		if (!process_layer_gpu::begin_process(gpu_texture, x_size, y_size))
		{
			std::cout << "process_layer_gpu::begin_process(*) failed\n";
			return false; // Signal fatal error
		}

		if (!filter_images)
		{
			auto error = false;
			new_frame = force_render || process_layer_gpu::is_new_pixels(error);
			if (error)
			{
				std::cout << "process_layer_gpu::is_new_pixels(error) failed\n";
				return false; // Signal fatal error
			}

			if (!new_frame)
			{
				process_layer_gpu::end_process();
				return true;
			}
		}

		if (!process_layer_gpu::set_image_area_data(image_area_data))
		{
			std::cout << "process_layer_gpu::set_image_area_data(*) failed";
			process_layer_gpu::end_process();
			return false; // Signal fatal error
		}


		if (dark_mode)
		{
			if (clock() - brightness_check_timer >= brightness_check_timer_interval)
			{
				auto error = false;
				pixels_bright = process_layer_gpu::is_current_pixels_bright(error);
				brightness_check_timer = clock();
			}

			if (!process_layer_gpu::invert_colors())
			{
				std::cout << "process_layer_gpu::invert_colors(*) failed\n";
				process_layer_gpu::end_process();
				return false; // Signal fatal error
			}
		}

		if (glass_mode)
		{
			if (!process_layer_gpu::glass_effect::map_shapes())
			{
				std::cout << "process_layer_gpu::glass_effect::map_shapes(*) failed";
				process_layer_gpu::end_process();
				return false; // Signal fatal error
			}
		}

		process_layer_gpu::end_process();
		return true;
	}

	/**
	 * \brief This function will process any given frame in the CPU only
	 * This function will fail if you never called to init_cpu_process_mode
	 * \param captured_texture - The given frame to process
	 * \param force_render - Use this flag to force reprocessing even if the frame
	 * is the exact frame as before
	 * \param new_frame - (OUT) This is output parameter that indicates if there
	 * was a new frame
	 * \return true in case no errors occurred, false in case there is error
	 */
	bool process_frame_in_cpu(ID3D11Texture2D* captured_texture, const bool force_render, bool& new_frame)
	{
		graphic_device::copy_texture(cpu_texture, captured_texture);

		if (!process_layer_cpu::begin_process(cpu_texture, x_size, y_size))
		{
			std::cout << "process_layer_cpu::begin_process(*) failed\n";
			return false; // Signal fatal error
		}

		new_frame = force_render || process_layer_cpu::is_new_pixels();

		if (!new_frame)
		{
			process_layer_cpu::end_process();
			return true;
		}

		if (filter_images)
			process_layer_cpu::map_images::map_images(force_render);

		if (dark_mode)
		{
			if (clock() - brightness_check_timer >= brightness_check_timer_interval)
			{
				pixels_bright = process_layer_cpu::is_current_pixels_bright();
				brightness_check_timer = clock();
			}

			process_layer_cpu::invert_colors();
		}


		if (glass_mode)
			process_layer_cpu::glass_effect::map_shapes(glass_background);

		process_layer_cpu::end_process();

		return true;
	}


	/**
	 * \brief This function is the implementation of the thread that working on
	 * reprocessing each frame that captured from the window.
	 * Never call directly to this function.
	 * To start running it, call to start_process_frame_thread function.
	 * To stop running it, call to stop_process_frame_thread function.
	 */
	void process_frame_thread()
	{
		process_frame_thread_exited = false;
		frame_thread_fatal_error = false;
		auto continue_run = true;

		if (start_processing_wait)
		{
			Sleep(1000);
			start_processing_wait = false;
		}

		while (run_process_frame_thread && continue_run)
		{
			if (was_maximized_timer || was_minimized_timer)
				break;

			if (is_target_window_in_use)
				Sleep(1);
			else
				Sleep(500);

			capture_layer::TextureData captured_frame = {nullptr};
			if (!capture_layer::get_new_frame(&captured_frame))
				continue;

			auto update_size = false;

			if (x_size != captured_frame.x_size || y_size != captured_frame.y_size)
			{
				force_render_timer = clock();


				if (resize_timer == 0)
				{
					display_layer::hide_target_hwnd();
					window_temporary_hidden = true;
				}
				resize_timer = clock();

				x_size = captured_frame.x_size;
				y_size = captured_frame.y_size;
			}


			if (resize_timer)
			{
				if (clock() - resize_timer >= 250)
				{
					update_size = true;
					resize_timer = 0;
				}
				else
				{
					continue;
				}
			}


			if (update_size)
			{
				display_layer::update_target_rect();
				display_layer::move_layer_to_target();

				if (graphic_device::is_cuda_adapter)
				{
					if (!init_gpu_process_mode(captured_frame.textrue))
					{
						std::cout << "init_gpu_process_mode(*) failed\n";
						fatal_error = true;
					}
				}
				else
				{
					if (!init_cpu_process_mode(captured_frame.textrue))
					{
						std::cout << "init_cpu_process_mode(*) failed\n";
						fatal_error = true;
					}
				}


				continue;
			}


			auto force_render = false;
			if (force_render_timer)
			{
				if (clock() - force_render_timer < 4000)
					force_render = true;
				else
					force_render_timer = 0;
			}


			// display_layer::draw_texture(captured_frame.textrue);

			auto new_frame = false;
			bool success;
			if (graphic_device::is_cuda_adapter)
			{
				success = process_frame_in_gpu(captured_frame.textrue, force_render, new_frame);
				if (success && new_frame)
					display_layer::draw_texture(gpu_texture);
			}
			else
			{
				success = process_frame_in_cpu(captured_frame.textrue, force_render, new_frame);
				if (success && new_frame)
					display_layer::draw_texture(cpu_texture);
			}


			if (!success)
			{
				frame_thread_fatal_error = true;
				continue_run = false;
			}

			if (window_temporary_hidden)
			{
				Sleep(100);
				display_layer::show_target_hwnd();
				window_temporary_hidden = false;
			}
		}

		process_frame_thread_exited = true;
	}


	/**
	 * \brief Enable dark mode
	 * \param filter_images - If set to true, it will not invert colors of images,
	 * otherwise it will invert everything
	 */
	void enable_dark_mode(const bool filter_images)
	{
		std::cout << "Enabling dark mode\n";
		startup_rendering = true;
		renderer::filter_images = filter_images;
		brightness_check_timer = clock();
		dark_mode = true;
	}

	/**
	 * \brief Disable dark mode
	 */
	void disable_dark_mode()
	{
		std::cout << "Disabling dark mode\n";
		un_init_for_target_hwnd();
		dark_mode = false;
	}


	/**
	 * \brief Enable glass effect
	 * \param filter_images - If set to true, it will try to avoid processing images as texts/shapes
	 * \param blur_level - The type of blur/transparency to set to the window
	 * \param brightness_level
	 * \param dark_background
	 * \param background_level
	 * \param images_level
	 * \param texts_level
	 * \return true on success, false on failure
	 */
	bool enable_glass_mode(const bool filter_images, const GlassBlurType blur_level, const double brightness_level,
	                       const bool dark_background, const double background_level, const double images_level,
	                       const double texts_level)
	{
		std::cout << "Enabling glass mode\n";
		// Set the variables
		glass_mode = true;
		glass_dark_background = dark_background; //dark_background;
		renderer::filter_images = filter_images;
		glass_blur_type = blur_level;
		glass_brightness_level = brightness_level;
		glass_background = background_level;
		glass_images = images_level;
		glass_texts = texts_level;
		startup_rendering = true;

		if (!init_for_target_hwnd())
		{
			glass_mode = false;
			std::cout << "Failed to enable glass mode\n";
			return false;
		}

		return true;
	}

	/**
	 * \brief Disable the glass mode
	 */
	void disable_glass_mode()
	{
		std::cout << "Disabling glass mode for " << target_hwnd;
		un_init_for_target_hwnd();
		glass_mode = false;
	}


	/**
	 * \brief Function will adjust the speed of the processing and reduce it in case
	 * the window is not in use
	 */
	void adjust_processing_speed()
	{
		const auto timer_interval = 1000;
		static clock_t timer = -timer_interval;

		if (clock() - timer < timer_interval)
			return;


		auto is_mouse_above_target_hwnd = []()
		{
			POINT point = {0};
			GetCursorPos(&point);

			auto hwnd = WindowFromPoint(point);
			if (!hwnd) return false;

			hwnd = GetAncestor(hwnd, GA_ROOT);

			return hwnd == target_hwnd;
		};

		auto is_target_window_active = []()
		{
			return GetForegroundWindow() == target_hwnd;
		};

		is_target_window_in_use = is_target_window_active() || is_mouse_above_target_hwnd();

		timer = clock();
	}

	/**
	 * \brief This function is used to handle events where the window was
	 * resized, maximized, minimized, moved.
	 * For example, this function will call to uninit_for_target_hwnd and init_for_target_hwnd
	 * when the window size changed or it created on the first time
	 * \param placement_changed - (OUT) output parameter that used not notify when
	 * there was event that there is need to reset the resources for processing
	 * \return true on success, false on failure
	 */
	bool process_window_placement(bool& placement_changed)
	{
		if (was_maximized_timer && clock() - was_maximized_timer >= 250)
		{
			placement_changed = true;

			stop_process_frame_thread();
			capture_layer::dispose();


			// Need to do it (ugly workaround) to avoid bug in WinRT capture API
			display_layer::update_target_rect();
			display_layer::target_rect.top -= 20;
			display_layer::target_rect.left -= 20;
			display_layer::move_layer_to_target();

			capture_layer::create_layer();
			capture_layer::start_capture_session();

			was_maximized_timer = 0;
			x_size = y_size = 0;
			start_process_frame_thread();

			return true;
		}

		if (was_minimized_timer && clock() - was_minimized_timer >= 500)
		{
			un_init_for_target_hwnd();
			placement_changed = true;
			window_hidden = true;
			was_minimized_timer = 0;
		}

		WINDOWPLACEMENT placement_new = {0};
		if (!GetWindowPlacement(target_hwnd, &placement_new))
		{
			std::cout << "Failed to get window placement. Window may be deleted";
			return false;
		}

		if (placement_new.showCmd != target_placement.showCmd)
		{
			const auto old_show_cmd = target_placement.showCmd;
			target_placement.showCmd = placement_new.showCmd;

			switch (placement_new.showCmd)
			{
			case SW_HIDE:
			case SW_MINIMIZE:
			case SW_SHOWMINIMIZED:
				std::cout << "Window is not on screen so suspending capturing\n";
				was_minimized_timer = clock();
				was_maximized_timer = 0;
				return true;

			case SW_SHOWNORMAL:
			case SW_RESTORE:
			case SW_MAXIMIZE:

				if (old_show_cmd == SW_MINIMIZE || old_show_cmd == SW_SHOWMINIMIZED)
					x_size = y_size = 0; // Prevent some bug when the window restored from minimized state

				was_minimized_timer = 0;
				
				if (!window_hidden)
				{
					was_maximized_timer = clock();	
				}
				else
				{
					Sleep(250);
					init_for_target_hwnd();
					window_hidden = false;
				}
				return true;
			}
		}

		if (placement_new.rcNormalPosition.left != target_placement.rcNormalPosition.left ||
			placement_new.rcNormalPosition.top != target_placement.rcNormalPosition.top)
		{
			display_layer::update_target_rect();
			display_layer::move_layer_to_target();
			target_placement.rcNormalPosition = placement_new.rcNormalPosition;
		}

		return true;
	}

	/**
	 * \brief This function will start the process frame thread when it is not
	 * running and when the brightness of the window is high.
	 * This function should be used only if the process frame thread was set to
	 * process dark mode effect.
	 */
	void process_dark_mode_brightness_check()
	{
		// Don't capture in case we not rendering anything
		if (window_hidden)
			return;

		// If the process frame running then skip
		if (process_frame_thread_exited)
		{
			// Check only few seconds
			if (clock() - brightness_check_timer < brightness_check_timer_interval)
				return;

			if (target_hwnd == GetForegroundWindow())
			{
				// In this case we will capture the window each few seconds
				// Using bitblt method, after that we will check if the pixels are bright
				// If they bright, we will run the process frame thread again to render 
				// the dark mode effect

				RECT target_rect = {0};
				DwmGetWindowAttribute(target_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &target_rect, sizeof(RECT));

				const int x_size = target_rect.right - target_rect.left + 1;
				const int y_size = target_rect.bottom - target_rect.top + 1;

				if (!capture_layer_bitblt::capture(target_rect.left, target_rect.top, x_size, y_size))
				{
					std::cout << "Failed to capture window using BitBlt API\n";
					brightness_check_timer = clock();
					return;
				}

				pixels_bright = process_layer_cpu::is_pixels_bright(capture_layer_bitblt::pixels, x_size, y_size);

				if (pixels_bright)
				{
					init_for_target_hwnd(true);
				}
			}

			brightness_check_timer = clock();
		}
		else if (!pixels_bright || frame_thread_fatal_error)
		{
			un_init_for_target_hwnd();
		}
	}

	/**
	 * \brief This function is used inside the main process thread where the
	 * UI also created
	 * \return true when there is no fatal error and false when there is fatal error.
	 * In case of false, the calling function should exit the whole program and/or at least
	 * print logs.
	 */
	bool process_loop()
	{
		if (fatal_error)
			return false;

		if (exit_event_requested)
		{
			if (rendering)
				un_init_for_target_hwnd();

			exit_event_requested = false;
			return false;
		}

		adjust_processing_speed();

		auto placement_changed = false;
		if (!process_window_placement(placement_changed))
			return false;

		return true;
	}
}
