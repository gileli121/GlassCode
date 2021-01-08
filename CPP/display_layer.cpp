#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#include "graphic_device.h"
#include <winrt/Windows.UI.h>
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Composition.Desktop.h>
#include <windows.ui.composition.interop.h>
#include <DispatcherQueue.h>
#include <plog/Log.h>

#include "display_layer.h"

#include <iostream>


namespace display_layer_helpers
{
	enum class dwm10_blur_accent
	{
		// Values passed to SetWindowCompositionAttribute determining the appearance of a window
		accent_disable = 0,
		accent_enable_gradient = 1,
		// Use a solid color specified by nColor. This mode ignores the alpha value and is fully opaque.
		accent_enable_transparentgradient = 2,
		// Use a tinted transparent overlay. nColor is the tint color.
		accent_enable_blurbehind = 3,
		// Use a tinted blurry overlay. nColor is the tint color.
		accent_enable_fluent = 4,
		// Use an aspect similar to Fluent design. nColor is tint color. This mode bugs if the alpha value is 0.
		accent_normal = 150 // (Fake value) Emulate regular taskbar appearance
	};


	void dwm10_set_window_blur(const HWND hwnd,
	                           const dwm10_blur_accent accent_state = dwm10_blur_accent::accent_enable_blurbehind,
	                           const int32_t flags = 0, const uint32_t color = 0, const int32_t animation_id = 0)
	{
		const HINSTANCE hModule = LoadLibrary(TEXT("user32.dll"));
		if (!hModule) return;


		struct WINCOMPATTRDATA // Composition Attributes
		{
			int nAttribute; // Type of the data struct passed
			void* pData; // Opaque pointer to the data struct (ACCENTPOLICY in our case)
			uint32_t ulDataSize; // Size of data struct
		};

		typedef BOOL (WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
		const pSetWindowCompositionAttribute SetWindowCompositionAttribute = (pSetWindowCompositionAttribute)
			GetProcAddress(hModule, "SetWindowCompositionAttribute");

		if (SetWindowCompositionAttribute)
		{
			struct ACCENTPOLICY // Determines how a window's transparent region will be painted
			{
				dwm10_blur_accent accent_state; // Appearance
				int32_t flags; // Nobody knows how this value works
				uint32_t color; // A color in the hex format AABBGGRR
				int32_t animation_id; // Nobody knows how this value works
			};

			ACCENTPOLICY policy = {accent_state, flags, color, animation_id};
			WINCOMPATTRDATA data = {19, &policy, sizeof(WINCOMPATTRDATA)};
			SetWindowCompositionAttribute(hwnd, &data);
		}
		FreeLibrary(hModule);
	}


	inline auto create_composition_surface_for_swap_chain(
		winrt::Windows::UI::Composition::Compositor const& compositor,
		::IUnknown* swap_chain)
	{
		winrt::Windows::UI::Composition::ICompositionSurface surface{nullptr};
		auto compositorInterop = compositor.as<ABI::Windows::UI::Composition::ICompositorInterop>();
		winrt::com_ptr<ABI::Windows::UI::Composition::ICompositionSurface> surface_interop;
		winrt::check_hresult(
			compositorInterop->CreateCompositionSurfaceForSwapChain(swap_chain, surface_interop.put()));
		winrt::check_hresult(surface_interop->QueryInterface(
			winrt::guid_of<winrt::Windows::UI::Composition::ICompositionSurface>(),
			reinterpret_cast<void**>(winrt::put_abi(surface))));
		return surface;
	}
}

namespace display_layer
{
	const char class_name[] = "WINTOP_DLAYER";

	HINSTANCE instance = nullptr;

	HWND target_hwnd = nullptr;
	// RECT target_rect = { 0 }; // Moved to global
	LONG target_orig_style = NULL;
	BYTE target_orig_alpha = 255;

	HWND display_hwnd = nullptr;
	HWND display_hwnd2 = nullptr;
	HWND display_hwnd_mask = nullptr;

	BlurType blur_type = BlurType::NONE;
	int transparent_level = 0;

	winrt::Windows::UI::Composition::Compositor display_compositor{nullptr};
	winrt::Windows::UI::Composition::ContainerVisual container_root{nullptr};
	winrt::Windows::UI::Composition::Desktop::DesktopWindowTarget desktop_window_target{nullptr};
	winrt::Windows::UI::Composition::SpriteVisual render_element{nullptr};
	winrt::Windows::UI::Composition::CompositionSurfaceBrush brush{nullptr};


	bool init()
	{
		std::cout << "Initializing display_layer\n";

		instance = GetModuleHandle(NULL);

		WNDCLASSEX wndclassex = {0};
		wndclassex.cbSize = sizeof(WNDCLASSEX);
		wndclassex.style = 0;
		wndclassex.lpfnWndProc = DefWindowProc;
		wndclassex.cbClsExtra = 0;
		wndclassex.cbWndExtra = 0;
		wndclassex.hInstance = instance;
		wndclassex.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
		wndclassex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wndclassex.hbrBackground = static_cast<HBRUSH>(CreateSolidBrush(RGB(0, 0, 0)));

		wndclassex.lpszMenuName = nullptr;

		wndclassex.lpszClassName = class_name;
		wndclassex.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

		if (!RegisterClassEx(&wndclassex))
		{
			std::cout << "Failed to register window class\n";
			return false;
		}


		return true;
	}

	void dispose()
	{
		// display_compositor.Close();
		// container_root.Close();
		// desktop_window_target.Close();
		// render_element.Close();
		// brush.Close();
		DestroyWindow(display_hwnd_mask);
		DestroyWindow(display_hwnd2);
		DestroyWindow(display_hwnd);

		if (target_orig_alpha > 0)
			SetLayeredWindowAttributes(target_hwnd, NULL, target_orig_alpha, LWA_ALPHA);
	}

	void set_target(const HWND target_hwnd)
	{
		display_layer::target_hwnd = target_hwnd;
	}

	void hide_target_hwnd()
	{
		SetWindowLong(target_hwnd, GWL_EXSTYLE, GetWindowLong(target_hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		DWORD flags = LWA_ALPHA;
		GetLayeredWindowAttributes(target_hwnd, nullptr, &target_orig_alpha, &flags);
		SetLayeredWindowAttributes(target_hwnd, NULL, 1, LWA_ALPHA);

		ShowWindow(display_hwnd, SW_HIDE);
		ShowWindow(display_hwnd2, SW_HIDE);
		ShowWindow(display_hwnd_mask, SW_HIDE);
	}

	void show_target_hwnd()
	{
		if (target_orig_alpha > 0)
			SetLayeredWindowAttributes(target_hwnd, NULL, target_orig_alpha, LWA_ALPHA);

		ShowWindow(display_hwnd, SW_SHOWNA);
		ShowWindow(display_hwnd2, SW_SHOWNA);
		ShowWindow(display_hwnd_mask, SW_SHOWNA);
	}

	void hide_layer()
	{
		ShowWindow(display_hwnd, SW_HIDE);
		ShowWindow(display_hwnd2, SW_HIDE);
		ShowWindow(display_hwnd_mask, SW_HIDE);
	}

	void show_layer()
	{
		ShowWindow(display_hwnd, SW_SHOWNA);
		ShowWindow(display_hwnd2, SW_SHOWNA);
		ShowWindow(display_hwnd_mask, SW_SHOWNA);
	}

	bool update_target_rect()
	{
		const auto res = DwmGetWindowAttribute(target_hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &target_rect, sizeof(RECT));
		if (res != S_OK)
		{
			std::stringstream ss;
			ss << "Failed to get window size for " << target_hwnd << " , HRESULT=" << res << std::endl;
			std::cout << ss.str();
			return false;
		}

		
		return true;
	}

	void move_layer_to_target()
	{
		
		SetWindowPos
		(
			display_hwnd,
			HWND_TOPMOST,
			target_rect.left,
			target_rect.top,
			target_rect.right - target_rect.left,
			target_rect.bottom - target_rect.top,
			SWP_NOSENDCHANGING | SWP_NOACTIVATE | SWP_NOZORDER
		);


		SetWindowPos
		(
			display_hwnd_mask,
			display_hwnd,
			target_rect.left,
			target_rect.top,
			target_rect.right - target_rect.left,
			target_rect.bottom - target_rect.top,
			SWP_NOSENDCHANGING | SWP_NOACTIVATE | SWP_NOZORDER
		);


		SetWindowPos
		(
			display_hwnd2,
			display_hwnd_mask,
			target_rect.left,
			target_rect.top,
			target_rect.right - target_rect.left,
			target_rect.bottom - target_rect.top,
			SWP_NOSENDCHANGING | SWP_NOACTIVATE | SWP_NOZORDER
		);
	}

	bool create_layer()
	{
		std::cout << "Creating layer\n";

		if (!update_target_rect())
		{
			std::cout << "Failed to get the size and position of the target window\n";
			return false;
		}

		display_hwnd = CreateWindowEx(WS_EX_NOACTIVATE | 0x080000 | WS_EX_TRANSPARENT | WS_EX_LAYERED , class_name,
		                              nullptr, 0x80000000 | WS_POPUP,
		                              target_rect.top, target_rect.left, target_rect.right - target_rect.left,
		                              target_rect.bottom - target_rect.top, target_hwnd, nullptr, instance, nullptr);

		if (!display_hwnd)
		{
			std::cout << "Failed to create layer window\n";
			return false;
		}


		ShowWindow(display_hwnd, SW_SHOWNOACTIVATE);


		display_hwnd_mask = CreateWindowEx(
			WS_EX_NOACTIVATE | 0x080000 | WS_EX_TRANSPARENT | WS_EX_LAYERED /*| WS_EX_TOPMOST*/ , class_name, nullptr,
			0x80000000 | WS_POPUP,
			0, 0, target_rect.right - target_rect.left, target_rect.bottom - target_rect.top, display_hwnd, nullptr,
			instance, nullptr);
		ShowWindow(display_hwnd_mask, SW_SHOWNOACTIVATE);

		display_hwnd2 = CreateWindowEx(
			WS_EX_NOACTIVATE | 0x080000 | WS_EX_TRANSPARENT | WS_EX_LAYERED /*| WS_EX_TOPMOST*/ , class_name, nullptr,
			0x80000000 | WS_POPUP,
			0, 0, target_rect.right - target_rect.left, target_rect.bottom - target_rect.top, display_hwnd_mask,
			nullptr, instance, nullptr);
		ShowWindow(display_hwnd2, SW_SHOWNOACTIVATE);


		move_layer_to_target();

		// I don't know why it is but I need this section to make it work
		winrt::Windows::System::DispatcherQueueController controller{nullptr};
		const DispatcherQueueOptions options
		{
			sizeof(DispatcherQueueOptions),
			DQTYPE_THREAD_CURRENT,
			DQTAT_COM_STA
		};
		CreateDispatcherQueueController(
			options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(winrt::put_abi(controller)));


		display_compositor = winrt::Windows::UI::Composition::Compositor();

		auto interop = display_compositor.as<ABI::Windows::UI::Composition::Desktop::ICompositorDesktopInterop>();

		const auto res = interop->CreateDesktopWindowTarget(display_hwnd2, true,
		                                                    reinterpret_cast<
			                                                    ABI::Windows::UI::Composition::Desktop::IDesktopWindowTarget
			                                                    **>(winrt::put_abi(desktop_window_target)));
		if (res != S_OK)
			return false;


		container_root = display_compositor.CreateContainerVisual();
		desktop_window_target.Root(container_root);


		render_element = display_compositor.CreateSpriteVisual();

		container_root.RelativeSizeAdjustment({1, 1});

		render_element.AnchorPoint({0.5f, 0.5f});
		render_element.RelativeOffsetAdjustment({0.5f, 0.5f, 0});
		render_element.RelativeSizeAdjustment({1, 1});


		// Create brush for the render element
		brush = display_compositor.CreateSurfaceBrush();
		render_element.Brush(brush);
		brush.HorizontalAlignmentRatio(0.5f);
		brush.VerticalAlignmentRatio(0.5f);
		brush.Stretch(winrt::Windows::UI::Composition::CompositionStretch::None);

		// Add the render element to the container root
		container_root.Children().InsertAtTop(render_element);


		return true;
	}

	
	void set_layer_to_foreground()
	{
		// Move the display layer in z-order to be above the target window
		SetForegroundWindow(target_hwnd);
		SetForegroundWindow(display_hwnd);
	}


	bool create_screen_buffer()
	{
		using namespace display_layer_helpers;

		//std::cout << "Initializing screen buffer for window\n";

		if (graphic_device::swap_chain)
		{
			std::cout << "Deleting existing swap chain\n";
			graphic_device::delete_swap_chain();
		}

		buffer.x_size = target_rect.right - target_rect.left;
		buffer.y_size = target_rect.bottom - target_rect.top;

		// Option 2 to create the swap chain - did not work perfect .....
		if (!graphic_device::create_swap_chain(buffer.x_size, buffer.y_size))
		{
			std::cout << "Failed to create swap chain for display layer\n";
		}


		// Next, use this swap chain
		const auto surface = create_composition_surface_for_swap_chain(display_compositor, graphic_device::swap_chain);
		brush.Surface(surface);


		return true;
	}

	bool set_target_window_transparent()
	{
		SetWindowLong(target_hwnd, GWL_EXSTYLE, GetWindowLong(target_hwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		DWORD flags = LWA_ALPHA;
		GetLayeredWindowAttributes(target_hwnd, nullptr, &target_orig_alpha, &flags);
		return SetLayeredWindowAttributes(target_hwnd, NULL, 1, LWA_ALPHA) != 0;
	}

	void un_set_target_window_transparent()
	{
		SetLayeredWindowAttributes(target_hwnd, NULL, target_orig_alpha, LWA_ALPHA) != 0;
	}

	void set_blur_type(const BlurType blur_type)
	{
		using namespace display_layer_helpers;

		//if (this->blurType == blurType) return;
		display_layer::blur_type = blur_type;

		dwm10_set_window_blur(display_hwnd, dwm10_blur_accent::accent_disable);

		switch (blur_type)
		{
		case BlurType::LOW:
			dwm10_set_window_blur(display_hwnd, dwm10_blur_accent::accent_enable_blurbehind);
			break;
		case BlurType::HIGH:
			dwm10_set_window_blur(display_hwnd, dwm10_blur_accent::accent_enable_fluent, 0, RGB(1, 1, 1));
			break;
		}
	}

	void set_brightness_level(const int level)
	{
		transparent_level = level;

		SetLayeredWindowAttributes(display_hwnd_mask, 0, 255 - level, LWA_ALPHA);
	}


	void draw_texture(ID3D11Texture2D* texture)
	{
		graphic_device::draw_texture_on_back_buffer(texture);
		DXGI_PRESENT_PARAMETERS presentParameters = {0};
		graphic_device::swap_chain->Present1(1, 0, &presentParameters);
	}
}
