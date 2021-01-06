#pragma once

#include <d3d11_4.h>
#include <winrt/base.h>
#include <winrt/Windows.Graphics.DirectX.Direct3d11.h>

#include "cuda_runtime.h"

namespace graphic_device
{
	struct GraphicDeviceMappedCPUTexture
	{
		byte* pixels = nullptr;
		int x_size = 0;
		int y_size = 0;
	};


	inline winrt::com_ptr<IDXGISwapChain1> com_ptr_swap_chain{nullptr};
	inline ID3D11Device* d3d_device = nullptr;
	inline IDXGIDevice* dxgi_device = nullptr;
	inline winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device{nullptr};
	inline IDXGISwapChain1* swap_chain{nullptr};
	inline ID3D11DeviceContext* d3d_context{nullptr};

	inline bool is_cuda_adapter = false;
	inline cudaDeviceProp cuda_device_prop_var = {0};


	void close();
	bool init_device(bool cuda_acceleration);
	bool create_swap_chain(int buffer_x_size, int buffer_y_size);
	void resize_swap_chain(int buffer_x_size, int buffer_y_size);
	void delete_swap_chain();
	bool create_texture(ID3D11Texture2D** texture,
	                        UINT access_flags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ,
	                        D3D11_USAGE usage = D3D11_USAGE_STAGING);
	void copy_texture(ID3D11Texture2D* dest_texture, ID3D11Texture2D* src_texture);
	bool get_mapped_cpu_texture(ID3D11Texture2D* cpu_access_texture, GraphicDeviceMappedCPUTexture* cpu_texture_data);
	void unmap_cpu_access_texture(ID3D11Texture2D* cpu_access_texture);
	void draw_texture_on_back_buffer(ID3D11Texture2D* texture);
}
