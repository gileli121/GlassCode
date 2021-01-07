#include <stdio.h>
#include <Windows.h>
#include <shellapi.h>
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/FuncMessageFormatter.h>
#include <plog/Initializers/RollingFileInitializer.h>

#include "renderer.h"

// forward declarations
LRESULT CALLBACK window_proc(
	_In_ HWND hwnd,
	_In_ UINT msg,
	_In_ WPARAM w_param,
	_In_ LPARAM l_param
);

#define ARGS_IS_CUDA_ENABLED 1
#define ARGS_WINDOW_HANDLE_IDX 2
#define ARGS_OPACITY_LEVEL_IDX 3
#define ARGS_BRIGHTNESS_LEVEL_IDX 4
#define ARGS_BLUR_TYPE_IDX 5
#define ARGS_COUNT 5


#define COMMAND_EXIT 0
#define COMMAND_SET_OPACITY 1
#define COMMAND_SET_BRIGHTNESS 2
#define COMMAND_SET_BLUR_TYPE 3


#ifndef _DEBUG
HWND target_hwnd = nullptr;
#else
HWND target_hwnd = reinterpret_cast<HWND>(0x00000000007C094C);
#endif

bool is_cuda_enabled = false;
int opacity_level = 0;
int brightness_level = 40;
int blur_type = 0;

bool should_exit = false;

// Input arguments: WINDOW_HANDLE OPACITY_LEVEL BRIGHTNESS_LEVEL BLUR_TYPE
int main(const int argc, char* argv[])
{
	plog::init(plog::debug, new plog::ConsoleAppender<plog::FuncMessageFormatter>);

	// bool wait = true;
	// while (wait);

	#ifndef _DEBUG
	if (argc - 1 < ARGS_COUNT)
	{
		std::cout << "There are missing arguments. Need at least " << ARGS_COUNT << std::endl;
		return EXIT_FAILURE;
	}


	std::cout << "Reading arguments";
	target_hwnd = reinterpret_cast<HWND>(atoi(argv[ARGS_WINDOW_HANDLE_IDX]));
	is_cuda_enabled = atoi(argv[ARGS_IS_CUDA_ENABLED]) != 0;
	opacity_level = atoi(argv[ARGS_OPACITY_LEVEL_IDX]);
	brightness_level = atoi(argv[ARGS_BRIGHTNESS_LEVEL_IDX]);
	blur_type = atoi(argv[ARGS_BLUR_TYPE_IDX]);
	#endif


	std::cout << "Checking arguments" << std::endl;
	if (!target_hwnd)
	{
		std::cout << "Invalid window handle provided" << std::endl;
		return EXIT_FAILURE;
	}

	if (opacity_level < 0 || opacity_level > 100)
	{
		std::cout << "Invalid opacity level provided" << std::endl;
		return EXIT_FAILURE;
	}

	if (brightness_level < 0 || brightness_level > 100)
	{
		std::cout << "Invalid brightness level provided" << std::endl;
		return EXIT_FAILURE;
	}

	if (blur_type < 0 || blur_type > 2)
	{
		std::cout << "Invalid blur type provided" << std::endl;
		return EXIT_FAILURE;
	}


	std::cout << "Creating messages-only window" << std::endl;

	auto create_message_only_window = [&]()
	{
		static const char* class_name = "GLASSIDE_RENDERER_MSG_WIN";
		WNDCLASSEX wx = {};
		wx.cbSize = sizeof(WNDCLASSEX);
		wx.lpfnWndProc = window_proc; // function which will handle messages
		wx.hInstance = GetModuleHandle(nullptr);
		wx.lpszClassName = class_name;
		if (!RegisterClassEx(&wx))
		{
			std::cout << "Failed to register class for message-only window" << std::endl;
			return static_cast<HWND>(nullptr);
		}


		return CreateWindowEx(0, class_name, "GlassIDE MSG Window", 0, 0, 0,
		                      0, 0, HWND_MESSAGE, nullptr, wx.hInstance, nullptr);
	};


	auto msg_window = create_message_only_window();
	std::cout << "MSG_WINDOW=" << msg_window << std::endl;

	std::cout << "Creating a renderer window for the target window" << std::endl;
	if (!renderer::init(is_cuda_enabled))
	{
		std::cout << "Failed to init_frame the renderer" << std::endl;
		return EXIT_FAILURE;
	}

	renderer::set_target(target_hwnd);

	if (!renderer::enable_glass_mode
		(
			false,
			static_cast<renderer::GlassBlurType>(blur_type),
			brightness_level / 100.0,
			false,
			opacity_level / 100.0,
			0.0
		))
	{
		std::cout << "Failed to enable glass mode" << std::endl;
		return EXIT_FAILURE;
	}


	clock_t check_window_timer = clock();

	while (!should_exit && renderer::process_loop())
	{
		MSG msg = {nullptr};
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Sleep(10);
		}

		if (clock() - check_window_timer >= 1000)
		{
			if (!IsWindow(target_hwnd))
			{
				renderer::disable_glass_mode();
				should_exit = true;
			}

			check_window_timer = clock();
		}
	}

	return !renderer::have_fatal_error() ? EXIT_SUCCESS : EXIT_FAILURE;
}

struct s_request_command
{
	int command_id;
	int value1;
};


LRESULT CALLBACK window_proc(
	_In_ const HWND hwnd,
	_In_ const UINT msg,
	_In_ const WPARAM w_param,
	_In_ const LPARAM l_param
)
{
	if (msg == WM_COPYDATA)
	{
		const auto copy_data_struct = reinterpret_cast<COPYDATASTRUCT*>(l_param);

		const auto request = static_cast<s_request_command*>(copy_data_struct->lpData);

		switch (request->command_id)
		{
		case COMMAND_SET_OPACITY:
			break;
		case COMMAND_SET_BRIGHTNESS:

			break;
		case COMMAND_SET_BLUR_TYPE:
			renderer::set_glass_blur_level(static_cast<renderer::GlassBlurType>(request->value1));
			break;
		default:
		case COMMAND_EXIT:
			renderer::register_exit_event();
			break;
		}

		//std::cout << "Received Command: ";
	}


	return DefWindowProc(hwnd, msg, w_param, l_param);
}
