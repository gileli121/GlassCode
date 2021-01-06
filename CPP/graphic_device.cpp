#include "graphic_device.h"
#include <roerrorapi.h>
#include <unknwn.h>
#include <inspectable.h>
#include <winrt/windows.graphics.directx.direct3d11.h>
#include <wincodec.h>
#include <plog/Log.h>

#include "direct3d11.interop.h"
#pragma comment(lib, "D3D11.lib")
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib,"windowsapp.lib")

namespace graphic_device
{
	namespace helpers
	{
		inline HRESULT create_d_3d_device(D3D_DRIVER_TYPE const type, ID3D11Device** device, IDXGIAdapter* adapter)
		{
			const UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

			const auto res = D3D11CreateDevice(
				adapter,
				type,
				nullptr,
				flags,
				nullptr, 0,
				D3D11_SDK_VERSION,
				device,
				nullptr,
				nullptr);

			return res;
		}

		HRESULT create_d_3d_device(ID3D11Device** device, IDXGIAdapter* adapter)
		{
			HRESULT hr;
			if (adapter == nullptr)
				hr = create_d_3d_device(D3D_DRIVER_TYPE_HARDWARE, device, adapter);
			else
				hr = create_d_3d_device(D3D_DRIVER_TYPE_UNKNOWN, device, adapter);

			if (DXGI_ERROR_UNSUPPORTED == hr)
			{
				hr = create_d_3d_device(D3D_DRIVER_TYPE_WARP, device, adapter);
			}

			return hr;
		}


		inline auto
		create_dxgi_swap_chain(
			winrt::com_ptr<ID3D11Device> const& device,
			const DXGI_SWAP_CHAIN_DESC1* desc)
		{
			auto dxgi_device = device.as<IDXGIDevice2>();
			winrt::com_ptr<IDXGIAdapter> adapter;
			winrt::check_hresult(dxgi_device->GetParent(winrt::guid_of<IDXGIAdapter>(), adapter.put_void()));
			winrt::com_ptr<IDXGIFactory2> factory;
			winrt::check_hresult(adapter->GetParent(winrt::guid_of<IDXGIFactory2>(), factory.put_void()));

			winrt::com_ptr<IDXGISwapChain1> swapchain;
			winrt::check_hresult(factory->CreateSwapChainForComposition(
				device.get(),
				desc,
				nullptr,
				swapchain.put()));

			return swapchain;
		}

		inline auto
		create_dxgi_swap_chain(
			winrt::com_ptr<ID3D11Device> const& device,
			const uint32_t width,
			const uint32_t height,
			const DXGI_FORMAT format,
			const uint32_t buffer_count)
		{
			DXGI_SWAP_CHAIN_DESC1 desc = {};
			desc.Width = width;
			desc.Height = height;
			desc.Format = format;
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.BufferCount = buffer_count;
			desc.Scaling = DXGI_SCALING_STRETCH;
			desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;

			return create_dxgi_swap_chain(device, &desc);
		}
	}

	void close()
	{
		if (swap_chain != nullptr)
		{
			swap_chain->Release();
			swap_chain = nullptr;
			com_ptr_swap_chain = nullptr;
		}

		if (d3d_context != nullptr)
		{
			d3d_context->Release();
			d3d_context = nullptr;
		}

		if (dxgi_device != nullptr)
		{
			dxgi_device->Release();

			dxgi_device = nullptr;
		}

		if (d3d_device != nullptr)
		{
			d3d_device->Release();
			d3d_device = nullptr;
		}
	}

	bool init_device(const bool cuda_acceleration)
	{
		PLOGI << "Creating graphic device";

		// Iterate through the candidate adapters
		IDXGIFactory* p_factory;
		auto hr = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&p_factory));

		if (!SUCCEEDED(hr))
		{
			PLOGE << "No DXGI Factory created";
			close();
			return false;
		}


		auto device_id = 0;

		IDXGIAdapter* graphic_adapter = nullptr;
		while (true)
		{
			hr = p_factory->EnumAdapters(device_id, &graphic_adapter);
			if (FAILED(hr)) break;

			DXGI_ADAPTER_DESC desc = {0};
			graphic_adapter->GetDesc(&desc);


			if (cuda_acceleration && cudaGetDeviceProperties(&cuda_device_prop_var, device_id) == cudaSuccess)
			{
				is_cuda_adapter = true;
				break;
			}

			device_id++;
		}

		if (helpers::create_d_3d_device(&d3d_device, graphic_adapter) != S_OK)
		{
			PLOGE << "Failed to create d3d device using adapter " << graphic_adapter;
			close();
			return false;
		}


		if (d3d_device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgi_device)) != S_OK)
		{
			PLOGE << "Failed to get IDXGIDevice interface from the d3dDevice";
			close();
			return false;
		}


		d3d_device->GetImmediateContext(&d3d_context);
		if (d3d_context == nullptr)
		{
			PLOGE << "Failed to get d3dContext from the D3D device";
			close();
			return false;
		}

		device = direct3d11_interop::CreateDirect3DDevice(dxgi_device);
		if (device == nullptr)
		{
			PLOGE << "Failed to CreateDirect3DDevice(dxgiDevice)";
			close();
			return false;
		}

		return true;
	}

	bool create_swap_chain(const int buffer_x_size, const int buffer_y_size)
	{
		if (swap_chain)
			return true;

		PLOGI << "Creating swapchain for the graphic device";


		auto d3d_device = direct3d11_interop::GetDXGIInterfaceFromObject<ID3D11Device>(device);
		d3d_device->GetImmediateContext(&d3d_context);

		com_ptr_swap_chain = helpers::create_dxgi_swap_chain(
			d3d_device,
			static_cast<uint32_t>(buffer_x_size),
			static_cast<uint32_t>(buffer_y_size),
			static_cast<DXGI_FORMAT>(winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized),
			2);

		swap_chain = com_ptr_swap_chain.get();
		if (swap_chain == nullptr)
		{
			PLOGE << "Failed to create swapchain";
			return false;
		}

		return true;
	}


	void resize_swap_chain(const int buffer_x_size, const int buffer_y_size)
	{
		PLOGI << "Resizing swapchain";

		swap_chain->ResizeBuffers
		(
			2,
			static_cast<uint32_t>(buffer_x_size),
			static_cast<uint32_t>(buffer_y_size),
			static_cast<DXGI_FORMAT>(winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized),
			0
		);
	}

	void delete_swap_chain()
	{
		PLOGI << "Deleting swapchain";

		//swapChain->Release();
		swap_chain = nullptr;
		com_ptr_swap_chain = nullptr;
	}

	bool create_texture(ID3D11Texture2D** texture, const UINT access_flags, const D3D11_USAGE usage)
	{
		PLOGI << "Creating texture";

		winrt::com_ptr<ID3D11Texture2D> back_buffer;
		winrt::check_hresult(swap_chain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), back_buffer.put_void()));

		D3D11_TEXTURE2D_DESC desc;
		back_buffer->GetDesc(&desc);


		desc.CPUAccessFlags = access_flags;
		desc.Usage = usage;
		desc.BindFlags = 0;
		const auto hr2 = d3d_device->CreateTexture2D(
			&desc,
			nullptr,
			texture
		);
		if (hr2 != S_OK)
		{
			PLOGE << "Failed to create texture, access denied";
			return false;
		}

		return true;
	}

	void copy_texture(ID3D11Texture2D* dest_texture, ID3D11Texture2D* src_texture)
	{
		if (!dest_texture || !src_texture) return;
		
		try
		{
			d3d_context->CopyResource(dest_texture, src_texture);
		}
		catch (std::exception&)
		{
			PLOGE << "exception copy_texture";
		}
	}

	bool get_mapped_cpu_texture(ID3D11Texture2D* cpu_access_texture, GraphicDeviceMappedCPUTexture* cpu_texture_data)
	{
		// map the texture
		D3D11_MAPPED_SUBRESOURCE mapInfo;
		ZeroMemory(&mapInfo, sizeof(D3D11_MAPPED_SUBRESOURCE));

		const auto hr = d3d_context->Map
		(
			cpu_access_texture,
			0, // Subresource
			D3D11_MAP_WRITE_NO_OVERWRITE,
			D3D11_MAP_WRITE_DISCARD, // MapFlags
			&mapInfo
		);

		if (hr != S_OK)
		{
			PLOGE << "Failed to get mapped cpu texture";
			return false;
		}


		cpu_texture_data->x_size = mapInfo.RowPitch / 4;
		cpu_texture_data->y_size = mapInfo.DepthPitch / mapInfo.RowPitch;
		cpu_texture_data->pixels = static_cast<byte*>(mapInfo.pData);

		return true;
	}

	void unmap_cpu_access_texture(ID3D11Texture2D* cpu_access_texture)
	{
		d3d_context->Unmap(cpu_access_texture, 0);
	}

	void draw_texture_on_back_buffer(ID3D11Texture2D* texture)
	{
		winrt::com_ptr<ID3D11Texture2D> back_buffer;
		winrt::check_hresult(swap_chain->GetBuffer(0, winrt::guid_of<ID3D11Texture2D>(), back_buffer.put_void()));

		d3d_context->CopyResource(back_buffer.get(), texture);
	}
}
