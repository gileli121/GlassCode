#include <Windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <windows.graphics.capture.interop.h>
#include <windows.graphics.capture.h>
#include <windows.graphics.directx.direct3d11.interop.h>

#include <DispatcherQueue.h>
#include <d3d11.h>
#pragma comment(lib, "D3D11.lib")

#include "direct3d11.interop.h"
#include "graphic_device.h"
#include "capture_layer.h"

#include <iostream>

namespace capture_layer_helpers
{
	bool create_capture_item_for_window_internal(const HWND hwnd, IGraphicsCaptureItemInterop* interop_factory,
		winrt::Windows::Graphics::Capture::GraphicsCaptureItem* item)
	{
		_COM_Outptr_ const auto result = reinterpret_cast<void**>(winrt::put_abi(*item));

		__try
		{
			interop_factory->CreateForWindow(
				hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), result);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			winrt::detach_abi(*item);
			return false;
		};

		return true;
	}

	inline auto create_capture_item_for_window(const HWND hwnd,
		winrt::Windows::Graphics::Capture::GraphicsCaptureItem* item)
	{
		const auto activation_factory = winrt::get_activation_factory<
			winrt::Windows::Graphics::Capture::GraphicsCaptureItem>();
		const auto interop_factory = activation_factory.as<IGraphicsCaptureItemInterop>();

		//TestFunc(hwnd, interop_factory.get());
		//interop_factory->CreateForWindow(hwnd, winrt::guid_of<ABI::Windows::Graphics::Capture::IGraphicsCaptureItem>(), reinterpret_cast<void**>(winrt::put_abi(item)));
		if (create_capture_item_for_window_internal(hwnd, interop_factory.get(), item))
			return true;
		else
			return false;
	}
}

namespace capture_layer
{
	HWND target_hwnd = NULL;
	TextureData texture_data;

	// WinRT stuff for the logic that used to capture with Win 10 API
	winrt::Windows::Graphics::Capture::GraphicsCaptureItem capture_item = { nullptr };
	winrt::Windows::Graphics::SizeInt32 capture_last_size;
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool::FrameArrived_revoker frame_arrived_revoker;


	// The 3d device stuff that used for the logic to get the captured data as ID3D11Texture2D
	winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool frame_pool{ nullptr };
	winrt::Windows::Graphics::Capture::GraphicsCaptureSession capture_session{ nullptr };

	bool new_frame = false;
	bool first_frame = true;
	bool is_closed = true;
	clock_t resize_frame_timer = 0;

	
	//bool capturing = false; // Moved to global

	bool init()
	{
		init_apartment(winrt::apartment_type::single_threaded);

		winrt::Windows::System::DispatcherQueueController controller{ nullptr };
		const DispatcherQueueOptions options{ sizeof(DispatcherQueueOptions), DQTYPE_THREAD_CURRENT, DQTAT_COM_STA };
		const auto res = CreateDispatcherQueueController(
			options, reinterpret_cast<ABI::Windows::System::IDispatcherQueueController**>(winrt::put_abi(controller)));
		if (res != S_OK)
		{
			std::cout << "Failed to initialize capture layer, hresult=" << res << std::endl;
			return false;
		}

		return true;
	}


	void callback_on_frame_arrived(
		winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool const& sender,
		winrt::Windows::Foundation::IInspectable const& args)
	{
		if (!capturing) return;

#if 0
		static clock_t timer = clock();
		pp clock() - timer ee;
		timer = clock();
#endif

		using namespace winrt::Windows::Graphics::DirectX;
		using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
		using namespace capture_layer_helpers;

		auto new_size = false;
		const auto frame = sender.TryGetNextFrame();
		const auto frame_content_size = frame.ContentSize();

		new_size = false;
		if (frame_content_size.Width != capture_last_size.Width || frame_content_size.Height != capture_last_size.Height
			)
		{
			if (first_frame)
			{
				new_size = true;
				first_frame = false;
			}
			else
			{
				resize_frame_timer = clock();
				capture_last_size = frame_content_size;
			}

		}
		else if (resize_frame_timer && clock() - resize_frame_timer > 250)
		{
			resize_frame_timer = 0;
			new_size = true;
		}

		

		if (new_size)
		{
			// The thing we have been capturing has changed size.
			// We need to resize our swap chain first, then blit the pixels.
			// After we do that, retire the frame and then recreate our frame pool.
			
			graphic_device::resize_swap_chain(capture_last_size.Width, capture_last_size.Height);
			texture_data.x_size = capture_last_size.Width;
			texture_data.y_size = capture_last_size.Height;
		}


		const auto frame_surface = direct3d11_interop::GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());


		texture_data.textrue = frame_surface.get();


		new_frame = true;


		if (new_size)
		{
			if (!create_capture_item_for_window(target_hwnd, &capture_item))
				return;
			
			
			frame_pool.Recreate(
				graphic_device::device,
				DirectXPixelFormat::B8G8R8A8UIntNormalized,
				2,
				frame_content_size);
		}
	}

	void set_target(const HWND target_hwnd)
	{
		capture_layer::target_hwnd = target_hwnd;
	}

	bool create_layer()
	{
		using namespace winrt;
		using namespace winrt::Windows::Graphics::Capture;
		using namespace winrt::Windows::Graphics::DirectX;
		using namespace capture_layer_helpers;


		std::cout << "Start capturing window " << target_hwnd << std::endl;

		if (!create_capture_item_for_window(target_hwnd, &capture_item))
		{
			std::cout << "Failed to create capture item for window " << target_hwnd << std::endl;
			return false;
		}

		const auto size = capture_item.Size();

		// Create framepool, define pixel format (DXGI_FORMAT_B8G8R8A8_UNORM), and frame size. 

		frame_pool = Direct3D11CaptureFramePool::Create(
			graphic_device::device,
			DirectXPixelFormat::B8G8R8A8UIntNormalized,
			2,
			size);

		capture_session = frame_pool.CreateCaptureSession(capture_item);


		capture_last_size = size;
		frame_arrived_revoker = frame_pool.FrameArrived(auto_revoke, &callback_on_frame_arrived);

		is_closed = false;
		return true;
	}


	void stop_capture_session()
	{
		if (!capturing) return;
		frame_pool.Close();
		capture_session.Close();
		capturing = false;
	}

	void start_capture_session()
	{
		capture_session.StartCapture();
		capturing = true;
		capture_session.IsCursorCaptureEnabled(false);

		first_frame = true;
	}

	void dispose()
	{

		// stop_capture_session();

#if 1 // I gave up.. there is no way to dispose this shit. It will just crash... 
		capture_last_size = { 0 };

		if (!is_closed)
		{
			frame_arrived_revoker.revoke();
			is_closed = true;
		}

		if (capturing)
		{
			stop_capture_session();
		}

		frame_pool = nullptr;
		capture_session = nullptr;
		capture_item = nullptr;
		texture_data = { nullptr };
		capturing = false;
		new_frame = false;
		first_frame = true;
#endif
	}

	void reload()
	{
		// dispose();
		// create_layer();

		start_capture_session();
	}

	bool get_new_frame(TextureData* texture_data)
	{
		if (!new_frame)
			return false;

		new_frame = true;
		*texture_data = capture_layer::texture_data;
		return true;
	}
}
