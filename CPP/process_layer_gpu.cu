// Cuda base


#include "cuda_runtime.h"

#include "device_launch_parameters.h"
#define __CUDACC__
#include <cuda_runtime_api.h>
#include <cuda_d3d11_interop.h>
#include <device_functions.h>
#include <plog/Log.h>

#include "process_layer_gpu.h"

#include <psapi.h>


#define DARK_MODE_WARP_SIZE 32
#define GLASS_MODE_WARP_SIZE 25
#define GLASS_MODE_WARP_SIZE_SQRT 5
#define GLASS_MODE_COLOR_DIV 11


#define CudaCheckError(ans) { gpu_assert((ans), __FILE__, __LINE__); }

inline void gpu_assert(const cudaError_t code, const char* file, const int line, const bool abort = false)
{
	if (code != cudaSuccess)
	{
		printf("gpu_assert: %s %s %d\n", cudaGetErrorString(code), file, line);
		if (abort)
			exit(code);
	}
}

namespace process_layer_gpu
{
	// ID3D11DeviceContext* d3d_context = nullptr; // TODO: Maybe remove this

	int x_size, y_size; // x and y size of the texture
	int x_end, y_end; // x and y size of the frame inside the texture
	bool* d_image_area_data = nullptr;
	unsigned char* d_pixels = nullptr;
	unsigned char* d_cached_pixels = nullptr;
	cudaArray* cu_array = nullptr;

	bool is_enable_cached_buffer = false;
	cudaGraphicsResource* cuda_resource = nullptr;

	bool* d_is_new_pixels = nullptr;
	int* d_bright_pixels_count = nullptr;

	ID3D11DeviceContext* d3d_context{nullptr};

	namespace glass_effect
	{
		bool is_enabled = false;

		unsigned char* pixels_reduced = nullptr;
		unsigned char* d_pixels_reduced = nullptr;

		int x_size_reduced;
		int y_size_reduced;
		int xy_size_reduced;

		double background_level;
		double dark_background;
		double images_level;
		double shapes_level;


		void enable(const double glass_background, const bool glass_dark_background, const double glass_images,
		            const double glass_shapes)
		{
			glass_effect::background_level = glass_background;
			glass_effect::dark_background = glass_dark_background;
			glass_effect::images_level = glass_images;
			glass_effect::shapes_level = glass_shapes;

			is_enabled = true;
		}

		void disable()
		{
			is_enabled = false;
		}

		void dispose()
		{
			// Free GPU memory
			if (d_pixels_reduced)
			{
				cudaFree(d_pixels_reduced);
				d_pixels_reduced = nullptr;
			}

			// Free CPU memory
			if (pixels_reduced)
			{
				free(pixels_reduced);
				pixels_reduced = nullptr;
			}
		}

		bool init()
		{
			auto on_error = [&](const char* error_message)
			{
				PLOGE << "load_frame failed: " << error_message;

				dispose();
				return nullptr;
			};

			dispose();

			// Init variables
			x_size_reduced = x_end / GLASS_MODE_WARP_SIZE_SQRT + 1;
			y_size_reduced = y_end / GLASS_MODE_WARP_SIZE_SQRT + 1;
			xy_size_reduced = x_size_reduced * y_size_reduced;

			// Allocate memory

			// Allocate memory in GPU
			const auto result = cudaMalloc(&d_pixels_reduced, sizeof(unsigned char) * xy_size_reduced);

			if (result != cudaSuccess)
				return on_error("Failed to malloc d_pixels_reduced on GPU");

			// Allocate memory in CPU
			pixels_reduced = static_cast<unsigned char*>(malloc(sizeof(unsigned char) * xy_size_reduced));
			if (pixels_reduced == nullptr) return on_error("Failed to malloc pixels_reduced on CPU");

			return true;
		}


		__global__ void kernel_perform_images_opacity(unsigned char* pixels, int x_size, int y_size,
		                                              const int xy_size,
		                                              const bool* image_area,
		                                              const float images_level)
		{
			const auto thread_4_point = blockIdx.x * blockDim.x + threadIdx.x;
			const auto thread_point = thread_4_point >> 2;

			if (thread_point >= xy_size) return;
			if (!image_area[thread_point]) return;

			pixels[thread_4_point] *= images_level;
		}


		__global__ void kernel_build_reduced_pixels(const unsigned char* pixels, const int x_size,
		                                            const int xy_size, unsigned char* pixels_reduced,
		                                            const int x_reduced)
		{
			// Calculate the points on the screen based on thread and block id
			const auto block_x = (blockIdx.x * GLASS_MODE_WARP_SIZE_SQRT) % x_size;
			const auto block_y = ((blockIdx.x * GLASS_MODE_WARP_SIZE_SQRT) / x_size) * GLASS_MODE_WARP_SIZE_SQRT;
			const auto thread_x = threadIdx.x % GLASS_MODE_WARP_SIZE_SQRT;
			const auto thread_y = threadIdx.x / GLASS_MODE_WARP_SIZE_SQRT;


			const int block_point = x_size * block_y + block_x;
			int thread_point = block_point + thread_y * x_size + thread_x;


			thread_point <<= 2;
			// End


			__shared__ int colors[GLASS_MODE_WARP_SIZE];

			// Mark common colors of these pixels

			colors[threadIdx.x] = 0;
			__syncthreads();


			if ((thread_point >> 2) < xy_size)
			{
				const unsigned char avg_color = (pixels[thread_point] + pixels[thread_point + 1] + pixels[thread_point +
						2]) /
					3 / GLASS_MODE_COLOR_DIV; // calculate the avg color
				atomicAdd(&colors[avg_color], 1);


				__syncthreads();

				__shared__ unsigned char most_common_color;


				if (threadIdx.x == 0)
				{
					most_common_color = 0;
					auto count = -1;
					for (auto i = 0; i < GLASS_MODE_WARP_SIZE; i++)
					{
						if (count == -1 || colors[i] > count)
						{
							most_common_color = i;
							count = colors[i];
						}
					}
				}


				__syncthreads();

				int point_r = ((block_y / GLASS_MODE_WARP_SIZE_SQRT) * x_reduced + (block_x /
					GLASS_MODE_WARP_SIZE_SQRT));

				pixels_reduced[point_r] = most_common_color;
			}
			//pixels[thread_point] = most_common_color;
			//pixels[thread_point + 1] = most_common_color;
			//pixels[thread_point + 2] = most_common_color;

			// End
		}


		void reduce_noise(unsigned char* pixels_reduced, const int x_reduced, const int y_reduced,
		                  int xy_reduced)
		{
			//int colors[64] = { 0 };
			//for (int y = 0; y < y_reduced; y++)
			//	for (int x = 0; x < x_reduced; x++)
			//		colors[pixels_reduced[y * x_reduced + x]]++;


			const auto common_color_min_count = 50;
			const auto processed_pixel_flag = 128;

			//unsigned int xa_size = y_reduced * x_reduced;

			unsigned int xa_size = y_reduced * x_reduced;

			auto process = [&](int point, const int point_max, const int point_jump, const int max_count)
			{
				const auto point_start = point;
				auto point_end = point;
				const auto color = pixels_reduced[point];

				auto count = 0;
				for (; point < point_max; point += point_jump)
				{
					if (color == pixels_reduced[point])
					{
						point_end = point;
						count = 0;
					}
					else if (++count >= max_count)
					{
						break;
					}
				}

				if (point_start < point_end)
				{
					for (auto point_2 = point_start; point_2 <= point_end; point_2 += point_jump)
						pixels_reduced[point_2] = color;
				}


				return point_end;
			};


			for (auto y = 0; y < y_reduced - 1; y++)
			{
				auto point = y * x_reduced;
				const auto point_max = point + x_reduced - 1;
				while (point < point_max)
					point = process(point, point_max, 1, 5) + 1;
			}

			for (auto x = 0; x < x_reduced; x++)
			{
				auto point = x;
				const auto point_max = x + (y_reduced - 1) * x_reduced;
				while (point < point_max)
					point = process(point, point_max, x_reduced, 4) + x_reduced;
			}
		}


		__global__ void kernel_mark_shapes(unsigned char* pixels_reduced, const int x_reduced,
		                                   int y_reduced,
		                                   unsigned char* pixels,
		                                   const int x_size, int y_size,
		                                   const int xy_size,
		                                   const int x_end, const int y_end,
		                                   bool* image_area_data, const float texts_level,
		                                   const float background_level, const bool dark_background)
		{
			auto block_x = (blockIdx.x * GLASS_MODE_WARP_SIZE_SQRT) % x_end;
			block_x -= block_x % GLASS_MODE_WARP_SIZE_SQRT;
			const auto thread_x = threadIdx.x % GLASS_MODE_WARP_SIZE_SQRT;
			if (block_x + thread_x >= x_end) return;


			auto block_y = ((blockIdx.x * GLASS_MODE_WARP_SIZE_SQRT) / x_end) * GLASS_MODE_WARP_SIZE_SQRT;
			block_y -= block_y % GLASS_MODE_WARP_SIZE_SQRT;
			const auto thread_y = threadIdx.x / GLASS_MODE_WARP_SIZE_SQRT;
			if (block_y + thread_y >= y_end) return;


			if (image_area_data && image_area_data[(block_y + thread_y) * x_size + (block_x + thread_x)]) 
				return;


			const auto block_point = block_y * x_end + block_x;
			auto thread_point = block_point + thread_y * x_end + thread_x;

			const auto y_point = thread_point / x_end;
			const auto x_point = thread_point % x_end;

			thread_point *= 4;


			__shared__ int avg_shapes_avg_color;
			__shared__ int avg_shapes_count;
			if (threadIdx.x == 0)
			{
				avg_shapes_avg_color = 0;
				avg_shapes_count = 0;
			}

			__syncthreads();

			const auto reduced_point = (y_point / GLASS_MODE_WARP_SIZE_SQRT) * x_reduced + (x_point /
				GLASS_MODE_WARP_SIZE_SQRT);

			unsigned char avg_color = (((pixels[thread_point] + pixels[thread_point + 1] + pixels[thread_point + 2]) / 3
			) / GLASS_MODE_COLOR_DIV) * GLASS_MODE_COLOR_DIV;
			const unsigned char reduced_color = pixels_reduced[reduced_point] * GLASS_MODE_COLOR_DIV;

			const auto is_shape_color = avg_color != reduced_color;


			if (is_shape_color)
			{
				atomicAdd(&avg_shapes_count, 1);
				atomicAdd(&avg_shapes_avg_color, avg_color);
			}

			__syncthreads();

			if (threadIdx.x == 0)
			{
				avg_shapes_avg_color /= avg_shapes_count;
			}

			__syncthreads();


			if (is_shape_color)
			{
				if (avg_shapes_avg_color < reduced_color)
				{
					pixels[thread_point] = ~pixels[thread_point];
					pixels[thread_point + 1] = ~pixels[thread_point + 1];
					pixels[thread_point + 2] = ~pixels[thread_point + 2];

					avg_color = (((pixels[thread_point] + pixels[thread_point + 1] + pixels[thread_point + 2]) / 3) /
						GLASS_MODE_COLOR_DIV) * GLASS_MODE_COLOR_DIV;
				}
			}
			else
			{
				//pixels[thread_point] = pixels[thread_point + 1] = pixels[thread_point + 2] = pixels[thread_point + 3] = 0;

				if (dark_background)
				{
					if (reduced_color > 128)
					{
						pixels[thread_point] = ~pixels[thread_point];
						pixels[thread_point + 1] = ~pixels[thread_point + 1];
						pixels[thread_point + 2] = ~pixels[thread_point + 2];
					}
				}


				if (background_level != 0)
				{
					pixels[thread_point] *= background_level;
					pixels[thread_point + 1] *= background_level;
					pixels[thread_point + 2] *= background_level;
					pixels[thread_point + 3] = 255 * background_level;
				}
				else
				{
					memset(&pixels[thread_point], 0, sizeof(unsigned char) * 4);
					//pixels[thread_point] = pixels[thread_point + 1] = pixels[thread_point + 2] = pixels[thread_point + 3] = 0;
				}
			}

			__shared__ int shape_max_brightness;

			__syncthreads();
			if (threadIdx.x == 0)
			{
				shape_max_brightness = 0;
			}

			__syncthreads();

			if (is_shape_color)
			{
				atomicMax(&shape_max_brightness, avg_color);
			}

			__syncthreads();

			if (is_shape_color)
			{
				auto scalar = 255 / static_cast<float>(shape_max_brightness);

				avg_color *= scalar;
				//if (avg_color > 255) avg_color = 255;

				int b = pixels[thread_point];
				int g = pixels[thread_point + 1];
				int r = pixels[thread_point + 2];


				if (texts_level < 1.0)
				{
#if 0
					b *= texts_level;
					g *= texts_level;
					r *= texts_level;

#else
					scalar *= texts_level;
#endif
				}

				b *= scalar;
				g *= scalar;
				r *= scalar;

				if (b > 255) b = 255;
				if (g > 255) g = 255;
				if (r > 255) r = 255;


				pixels[thread_point] = b;
				pixels[thread_point + 1] = g;
				pixels[thread_point + 2] = r;
			}
		}


		bool map_shapes()
		{
			cudaError_t cuda_result;
			if (images_level < 1.0)
			{
				kernel_perform_images_opacity
					<< < ((y_end) * (x_end) * 4) / GLASS_MODE_WARP_SIZE, GLASS_MODE_WARP_SIZE >> >
					(d_pixels, x_end, y_end, (y_end - 1) * x_end,
					 d_image_area_data, images_level);

				cuda_result = cudaDeviceSynchronize();
				if (cuda_result != cudaSuccess)
				{
					CudaCheckError(cuda_result);
					return false;
				}
			}


			kernel_build_reduced_pixels
				<< < xy_size_reduced, GLASS_MODE_WARP_SIZE >> >
				(d_pixels, x_end, (y_end - 1) * x_end, d_pixels_reduced, x_size_reduced);


			cuda_result = cudaDeviceSynchronize();
			if (cuda_result != cudaSuccess)
			{
				CudaCheckError(cuda_result);
				return false;
			}


			cuda_result = cudaMemcpy(pixels_reduced, d_pixels_reduced, sizeof(unsigned char) * xy_size_reduced,
			                         cudaMemcpyDeviceToHost);
			if (cuda_result != cudaSuccess)
			{
				CudaCheckError(cuda_result);
				return false;
			}

			reduce_noise(pixels_reduced, x_size_reduced, y_size_reduced, xy_size_reduced);


#if 0 // Display reduced pixels (For debug only)
			for (int y = 0; y < data->y_end; y++)
				for (int x = 0; x < data->x_end; x++)
				{
					int point = (y * data->x_end + x) * 4;
					int point_r = ((y / GLASS_MODE_WARP_SIZE_SQRT) * data->x_size_reduced + (x / GLASS_MODE_WARP_SIZE_SQRT));

					pixels[point] = pixels[point + 1] = pixels[point + 2] = ((data->pixels_reduced[point_r]) & 63) * GLASS_MODE_COLOR_DIV;
				}

			return;
#endif

			cuda_result = cudaMemcpy(d_pixels_reduced, pixels_reduced, sizeof(unsigned char) * xy_size_reduced,
			                         cudaMemcpyHostToDevice);
			if (cuda_result != cudaSuccess)
			{
				CudaCheckError(cuda_result);
				return false;
			}


			// GPU-Process(pixels_reduced,pixels)
			kernel_mark_shapes
				<< < xy_size_reduced, GLASS_MODE_WARP_SIZE >> >
				(d_pixels_reduced, x_size_reduced, y_size_reduced, d_pixels, x_size, y_size,
				 x_size * (y_size - 1), x_end, y_end,
				 d_image_area_data, shapes_level, background_level, dark_background);


			cuda_result = cudaDeviceSynchronize();
			if (cuda_result != cudaSuccess)
			{
				CudaCheckError(cuda_result);
				return false;
			}


			return true;
		}
	}

	void init(ID3D11DeviceContext* d3d_context)
	{
		process_layer_gpu::d3d_context = d3d_context;
	}

	bool enable_cache_buffer(const bool enable)
	{
		is_enable_cached_buffer = enable;
		if (!enable)
		{
			if (d_cached_pixels)
			{
				cudaFree(d_cached_pixels);
				d_cached_pixels = nullptr;
			}

			if (d_is_new_pixels)
			{
				cudaFree(d_is_new_pixels);
				d_is_new_pixels = nullptr;
			}
		}
		else
		{
			if (!d_is_new_pixels)
			{
				const auto result = cudaMalloc(&d_is_new_pixels, sizeof(bool));
				if (result != cudaSuccess)
				{
					CudaCheckError(result);
					return false;
				}
			}
		}

		return true;
	}

	bool init_frame(int x_size, int y_size, int x_end, int y_end)
	{
		process_layer_gpu::x_end = x_size;
		process_layer_gpu::y_end = y_size;
		process_layer_gpu::x_end = x_end ? x_end : x_size;
		process_layer_gpu::y_end = y_end ? y_end : y_size;
		return true;
	}

	void set_default_settings()
	{
		glass_effect::is_enabled = false;
		is_enable_cached_buffer = false;
	}

	void free_resources()
	{
		if (d_pixels)
		{
			cudaFree(d_pixels);
			d_pixels = nullptr;
		}

		if (d_cached_pixels)
		{
			cudaFree(d_cached_pixels);
			d_cached_pixels = nullptr;
		}

		if (d_image_area_data)
		{
			cudaFree(d_image_area_data);
			d_image_area_data = nullptr;
		}

		if (cuda_resource)
		{
			cudaGraphicsUnregisterResource(cuda_resource);
			cuda_resource = nullptr;
		}

		x_size = y_size = x_end = y_end = 0;

		EmptyWorkingSet(GetCurrentProcess()); // Reduce memory usage
	}

	bool begin_process(ID3D11Texture2D* texture, const int capture_x_size, const int capture_y_size)
	{
#if 0
		D3D11_MAPPED_SUBRESOURCE map_info;
		ZeroMemory(&map_info, sizeof(D3D11_MAPPED_SUBRESOURCE));

		const auto hr = d3d_context->Map
		(
			texture,
			0, // Subresource
			D3D11_MAP_READ,
			0, // MapFlags
			&map_info
		);

		if (hr != S_OK)
		{
			PLOGE << "Failed to get mapped cpu texture";
			return false;
		}

		process_layer_cpu::texture = texture;
		return load_frame(static_cast<byte*>(map_info.pData), map_info.RowPitch / 4,
			map_info.DepthPitch / map_info.RowPitch,
			x_end, y_end);
#endif

		cudaError_t result;
		const auto is_resized = capture_x_size != x_end || capture_y_size != y_end;
		if (is_resized)
		{
			free_resources();

			x_end = capture_x_size;
			y_end = capture_y_size;


			D3D11_MAPPED_SUBRESOURCE map_info;
			ZeroMemory(&map_info, sizeof(D3D11_MAPPED_SUBRESOURCE));

			const auto hr = d3d_context->Map
			(
				texture,
				0, // Subresource
				D3D11_MAP_READ,
				0, // MapFlags
				&map_info
			);

			if (hr != S_OK)
			{
				PLOGE << "Failed to get mapped cpu texture";
				return false;
			}

			x_size = map_info.RowPitch / 4;
			y_size = map_info.DepthPitch / map_info.RowPitch;

			d3d_context->Unmap(texture, 0);


			result = cudaGraphicsD3D11RegisterResource(&cuda_resource, texture, cudaGraphicsRegisterFlagsNone);
			if (result != cudaSuccess)
			{
				CudaCheckError(result);
				return false;
			}

			result = cudaMalloc(&d_pixels, x_size * y_size * 4 * sizeof(unsigned char));
			if (result != cudaSuccess)
			{
				CudaCheckError(result);
				return false;
			}

			if (glass_effect::is_enabled)
				glass_effect::init();


			EmptyWorkingSet(GetCurrentProcess()); // Reduce memory usage
		}

		result = cudaGraphicsMapResources(1, &cuda_resource, nullptr);
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			return false;
		}


		result = cudaGraphicsSubResourceGetMappedArray(&cu_array, cuda_resource, 0, 0);
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			cudaGraphicsUnmapResources(1, &cuda_resource, nullptr);
			return false;
		}

		result = cudaMemcpyFromArray(d_pixels, cu_array, 0, 0, x_end * y_end * 4 * sizeof(unsigned char),
		                             cudaMemcpyDeviceToDevice);
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			cudaGraphicsUnmapResources(1, &cuda_resource, nullptr);
			// TODO - Add unmap here
			return false;
		}


		if (is_enable_cached_buffer && is_resized)
		{
			if (d_cached_pixels)
			{
				cudaFree(d_cached_pixels);
				d_cached_pixels = nullptr;
			}

			result = cudaMalloc(&d_cached_pixels, x_size * y_size * 4 * sizeof(unsigned char));
			if (result != cudaSuccess)
			{
				CudaCheckError(result);
				return false;
			}

			result = cudaMemcpy(d_cached_pixels, d_pixels, x_size * y_size * 4 * sizeof(unsigned char),
			                    cudaMemcpyDeviceToDevice);
			if (result != cudaSuccess)
			{
				CudaCheckError(result);
				return false;
			}
		}


		return true;
	}

	bool end_process()
	{
		auto result = cudaMemcpyToArray(cu_array, 0, 0, d_pixels, x_end * y_end * 4 * sizeof(unsigned char),
		                                cudaMemcpyDeviceToDevice);
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			return false;
		}

		result = cudaGraphicsUnmapResources(1, &cuda_resource, nullptr);
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			return false;
		}


		return true;
	}


	__global__ void kernel_is_new_pixels(bool* image_area_data, unsigned char* cached_pixels, unsigned char* pixels,
	                                     const int x_size, const int x_end, const int y_end, bool* is_new_pixels)
	{
		auto point = blockIdx.x * blockDim.x + threadIdx.x;
		if (point % x_size >= x_end)
			return;

		if (point / x_size >= y_end)
			return;

		point *= 4;

		auto _is_new_pixels = false;
		if (cached_pixels[point] != pixels[point])
		{
			cached_pixels[point] = pixels[point];
			_is_new_pixels = true;
		}

		if (cached_pixels[point + 1] != pixels[point + 1])
		{
			cached_pixels[point + 1] = pixels[point + 1];
			_is_new_pixels = true;
		}

		if (cached_pixels[point + 2] != pixels[point + 2])
		{
			cached_pixels[point + 2] = pixels[point + 2];
			_is_new_pixels = true;
		}

		if (_is_new_pixels && !(*is_new_pixels))
			*is_new_pixels = true;
	}

	bool is_new_pixels(bool& error)
	{
		auto result = cudaMemset(d_is_new_pixels, false, sizeof(bool));
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			error = true;
			return false;
		}

		kernel_is_new_pixels
			<< < (x_size * y_size) / DARK_MODE_WARP_SIZE, DARK_MODE_WARP_SIZE >> >
			(d_image_area_data, d_cached_pixels, d_pixels, x_size, x_end, y_end, d_is_new_pixels);

		result = cudaDeviceSynchronize();
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			error = true;
			return false;
		}

		bool is_new_pixels = false;
		result = cudaMemcpy(&is_new_pixels, d_is_new_pixels, sizeof(bool), cudaMemcpyDeviceToHost);
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			error = true;
			return false;
		}

		return is_new_pixels;
	}

	bool set_image_area_data(bool* image_area_data)
	{
		if (!image_area_data)
		{
			if (d_image_area_data)
			{
				cudaFree(d_image_area_data);
				d_image_area_data = nullptr;
			}
			return true;
		}

		if (!d_image_area_data)
		{
			const auto result = cudaMalloc(&d_image_area_data, x_size * y_size * sizeof(bool));
			if (result != cudaSuccess)
			{
				CudaCheckError(result);
				return false;
			}
		}

		const auto result = cudaMemcpy(d_image_area_data, image_area_data, x_size * y_size * sizeof(bool),
		                               cudaMemcpyHostToDevice);
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			return false;
		}

		return true;
	}

	__global__ void kernel_is_current_pixels_bright(unsigned char* pixels, bool* image_area_data, int* bright_count,
	                                                const int x_size, const int x_end, const int y_end)
	{
		auto point = blockIdx.x * blockDim.x + +threadIdx.x;;
		if (point % x_size >= x_end)
			return;

		if (point / x_size >= y_end)
			return;


		point *= 4;

		const auto color_avg = (pixels[point] + pixels[point + 1] + pixels[point + 2]) / 3;
		if (color_avg > 127)
			atomicAdd(bright_count, 1);
	}

	bool is_current_pixels_bright(bool& error)
	{
		if (!d_bright_pixels_count)
		{
			const auto result = cudaMalloc(&d_bright_pixels_count, sizeof(int));
			if (result != cudaSuccess)
			{
				CudaCheckError(result);
				error = true;
				return false;
			}
		}

		auto result = cudaMemset(d_bright_pixels_count, 0, sizeof(int));
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			error = true;
			return false;
		}


		kernel_is_current_pixels_bright
			<< < (x_size * y_size) / DARK_MODE_WARP_SIZE, DARK_MODE_WARP_SIZE >> >
			(d_pixels, d_image_area_data, d_bright_pixels_count, x_size, x_end, y_end);


		result = cudaDeviceSynchronize();
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			error = true;
			return false;
		}


		int bright_pixels_count = 0;
		result = cudaMemcpy(&bright_pixels_count, d_bright_pixels_count, sizeof(int), cudaMemcpyDeviceToHost);
		if (result != cudaSuccess)
		{
			CudaCheckError(result);
			error = true;
			return false;
		}


		return bright_pixels_count > x_end * y_end / 2;
	}


	__global__ void kernel_invert_colors
	(bool* d_image_area_data, unsigned char* d_pixels, const int x_size, const int x_end, const int y_end)
	{
		const auto idx = blockIdx.x * blockDim.x + threadIdx.x;


		const auto x = idx % x_size;
		const auto y = idx / x_size;

		if (x >= x_end || y >= y_end)
			return;


		if (d_image_area_data && d_image_area_data[idx])
			return;

		const auto point = (y * x_end + x) * 4;

		d_pixels[point] = ~d_pixels[point];
		d_pixels[point + 1] = ~d_pixels[point + 1];
		d_pixels[point + 2] = ~d_pixels[point + 2];
	}

	bool invert_colors()
	{
		//return true;
		kernel_invert_colors
			<< < (x_size * y_size) / DARK_MODE_WARP_SIZE, DARK_MODE_WARP_SIZE >> >
			(d_image_area_data, d_pixels, x_size, x_end, y_end);


		const auto result = cudaDeviceSynchronize();
		if (result == cudaSuccess)
			return true;

		CudaCheckError(result);
		return false;
	}
}
