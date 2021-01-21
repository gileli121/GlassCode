#include <d3d11.h>
#include <plog/Log.h>

#include "process_layer_cpu.h"


#include <iostream>
#include <psapi.h>


namespace process_layer_cpu
{
	// The x_end, y_end and x_end*y_end of the buffers 
	int x_size = 0, y_size = 0, xy_size = 0;
	int x_end = 0, y_end = 0;

	// The pixels and texture
	ID3D11Device* d3d_device = nullptr;
	byte* pixels = nullptr;
	byte* cached_pixels = nullptr;
	ID3D11Texture2D* texture = nullptr;
	ID3D11DeviceContext* d3d_context = nullptr;

	bool is_enable_cached_buffer = false;


	// The sizes of the buffers in different way...
	int xb_size = 0, xb_size0_b = 0;
	int xa_size = 0;
	int xab_size = 0;

	// Points that specify the boundary of the pixels
	int xa_start, xa_end;
	int xb_start, xb_end;


#define GET_XA(point) (((point) / xb_size) * xb_size)
#define GET_XB(point) ((point) - GET_XA(point))

	namespace map_images
	{
		bool is_enabled = false;

		// Amount of pixels to skip for few image process functions
		int img_proc_xa_skip, img_proc_xb_skip;
		// Amount of pixels to skip of the isImageArea function
		int is_img_area_xa_skip, is_img_area_xb_skip;

		// Map array that map the area of each shapes groups
		bool* image_area_data = nullptr;

		// Map array of common colors that used for detecting images
		bool common_colors[256] = {false};

		// Timer about when to update the common color data
		constexpr int update_common_color_interval = 5000;
		clock_t update_common_colors_timer = 0;


		constexpr double is_image_area_grid = 0.0028116213683224;
		constexpr int is_image_area_grid_points = 4;

		void enable()
		{
			is_enabled = true;
		}

		void disable()
		{
			is_enabled = false;
		}

		void free_resources()
		{
			if (image_area_data)
			{
				free(image_area_data);
				image_area_data = nullptr;
			}
		}

		bool init()
		{
			free_resources();

			image_area_data = static_cast<bool*>(malloc(xy_size * sizeof(bool)));
			if (!image_area_data)
			{
				std::cout << "Failed to malloc CPU memory for d_image_area_data\n";
				return false;
			}

			RECT rect_screen_size = {0};
			GetWindowRect(GetDesktopWindow(), &rect_screen_size);

			const int xy_screen_size = rect_screen_size.right + rect_screen_size.bottom;


			// IsImageArea grid
			const int tmp = is_image_area_grid * xy_screen_size;
			is_img_area_xa_skip = tmp * xb_size;
			is_img_area_xb_skip = tmp * 4;

			// Update image poses grid
			img_proc_xa_skip = is_img_area_xa_skip * is_image_area_grid_points;
			img_proc_xb_skip = is_img_area_xb_skip * is_image_area_grid_points;

			memset(image_area_data, false, xy_size * sizeof(bool));

			update_common_colors_timer = -update_common_color_interval;
			return true;
		}

		void update_common_colors()
		{
			auto add_color = [&](const byte* color)
			{
				common_colors[(color[2] + color[1] + color[0]) / 3] = true;
			};

			const auto y_jump = 4;
			const auto y_down_rate = 32;
			const auto update_common_colors_max_repeat_count = 200;

			unsigned int count = 0, xy_jump, point;
			byte* color;
			for (unsigned int y = 0; y < y_size; y += y_jump)
			{
				xy_jump = 0;
				count = 1;
				color = &pixels[y * xb_size];

				for (unsigned int x = 1; x < x_size; x++)
				{
					if (x % y_down_rate == 0)
					{
						xy_jump++;
						if (y + xy_jump >= y_size) break;
					}

					point = ((y + xy_jump) * x_size + x) * 4;
					if (color[0] == pixels[point] && color[1] == pixels[point + 1] && color[2] == pixels[point + 2])
					{
						count++;
						if (count >= update_common_colors_max_repeat_count)
						{
							add_color(color);

							count = 1;
						}
					}
					else
					{
						count = 1;
						color = &pixels[point];
					}
				}
			}

			for (auto tx_start = y_jump * y_down_rate; tx_start < x_size; tx_start += y_jump * y_down_rate)
			{
				xy_jump = 0;
				count = 1;
				color = &pixels[tx_start * 4];
				for (auto x = tx_start + 1; x < x_size; x++)
				{
					if ((x - tx_start) % y_down_rate == 0)
					{
						xy_jump++;
						if (xy_jump >= y_size) break;
					}
					point = xy_jump * xb_size + x * 4;

					if (color[0] == pixels[point] && color[1] == pixels[point + 1] && color[2] == pixels[point + 2])
					{
						count++;
						if (count >= update_common_colors_max_repeat_count)
						{
							add_color(color);
							count = 1;
						}
					}
					else
					{
						count = 1;
						color = &pixels[point];
					}
				}
			}


			for (auto x = x_size - 1; x >= 0; x -= y_jump)
			{
				xy_jump = 0;
				count = 1;
				color = &pixels[x * 4];

				for (unsigned int y = 1; y < y_size - 1; y++)
				{
					if (y % y_down_rate == 0)
					{
						xy_jump++;
						if (xy_jump >= x) break;
					}


					point = (y * x_size + x - xy_jump) * 4;

					if (color[0] == pixels[point] && color[1] == pixels[point + 1] && color[2] == pixels[point + 2])
					{
						count++;
						if (count >= update_common_colors_max_repeat_count)
						{
							add_color(color);
							count = 1;
						}
					}
					else
					{
						count = 1;
						color = &pixels[point];
					}
				}
			}

			for (auto ty_start = y_jump * y_down_rate; ty_start < y_size; ty_start += y_jump * y_down_rate)
			{
				xy_jump = 0;

				color = &pixels[(ty_start * x_size + x_size - 1) * 4];

				for (auto y = ty_start + 1; y < y_size; y++)
				{
					if ((y - ty_start) % y_down_rate == 0)
					{
						xy_jump++;
						if (xy_jump >= x_size) break;
					}

					point = (y * x_size + x_size - 1 - xy_jump) * 4;

					if (color[0] == pixels[point] && color[1] == pixels[point + 1] && color[2] == pixels[point + 2])
					{
						count++;
						if (count >= update_common_colors_max_repeat_count)
						{
							add_color(color);
							count = 1;
						}
					}
					else
					{
						count = 1;
						color = &pixels[point];
					}
				}
			}
		}

		bool* map_images(bool force_update_common_colors)
		{
			if (force_update_common_colors || clock() - update_common_colors_timer >= update_common_color_interval)
			{
				update_common_colors();
				update_common_colors_timer = clock();
			}

			memset(image_area_data, false, sizeof(bool) * xy_size);


			auto is_image_area = [&](const int point, int level = 5)
			{
				const auto xa_skip = is_img_area_xa_skip;
				const auto xb_skip = is_img_area_xb_skip;

				const int t_xa_start = GET_XA(point) - (xa_skip * (is_image_area_grid_points * 0.5));
				const int t_xb_start = GET_XB(point) - (xb_skip * (is_image_area_grid_points * 0.5));


				if (t_xa_start < 0 || t_xb_start < 0)
					return false;


				auto t_xa_end = t_xa_start + is_img_area_xa_skip * (is_image_area_grid_points - 1);
				auto t_xb_end = t_xb_start + is_img_area_xb_skip * is_image_area_grid_points;

				if (t_xa_end > xa_size) t_xa_end = xa_size;
				if (t_xb_end > xb_size0_b) t_xb_end = xb_size0_b;

				auto count = 0;

				for (auto xa = t_xa_start; xa <= t_xa_end; xa += xa_skip)
				{
					auto* point1 = &pixels[xa + t_xb_start];
					for (auto xb = t_xb_start + xb_skip; xb <= t_xb_end; xb += xb_skip)
					{
						auto* const point2 = &pixels[xa + xb];

						if (point1[0] != point2[0] || point1[1] != point2[1] || point1[2] != point2[2])
						{
							count++;
							point1 = point2;
						}
						else
							count--;
					}
				}

				return count > level;
			};


			for (auto xa = xa_start; xa <= xa_end; xa += img_proc_xa_skip)
			{
				for (auto xb = xb_start; xb < xb_end; xb += img_proc_xb_skip)
				{
					auto point = xa + xb;

					if (image_area_data[point / 4]) continue;

					if (!is_image_area(point)) continue;

					auto point_a = point;
					auto point_b = -1;

					int xb2, xb1 = xb, xa1 = xa;

					for (xb += img_proc_xb_skip; xb < xb_end; xb += img_proc_xb_skip)
					{
						point = xa + xb;
						if (image_area_data[point / 4]) break;
						if (!is_image_area(point, 10)) break;
						point_b = point;
						xb2 = xb;
					}

					if (point_b == -1 || (xb2 - xb1) / img_proc_xb_skip <= 3) continue;

					auto point_c = -1, xa2 = -1;

					for (auto xb_2 = xb1; xb_2 < xb2; xb_2 += img_proc_xb_skip)
						for (auto xa_2 = xa1; xa_2 < xa_size; xa_2 += img_proc_xa_skip)
						{
							point = xa_2 + xb_2;
							if (image_area_data[point / 4]) break;
							if (!is_image_area(point)) break;
							if (xa_2 <= xa2) continue;
							xa2 = xa_2;
							point_c = point;
						}


					if (xa2 == -1) continue;


#if 1 // Filter 1 - PROCESS_LAYER_HANDLE_UNSTABLE_IMAGE

					while (true)
					{
						auto b_xb_size_new = false;

						for (auto xa_2 = xa1; xa_2 <= xa2; xa_2 += img_proc_xa_skip)
						{
							for (auto xb_2 = xb2 + img_proc_xb_skip; xb_2 <= xb_end; xb_2 += img_proc_xb_skip)
							{
								point = xa_2 + xb_2;
								if (image_area_data[point / 4]) break;
								if (!is_image_area(point, 10)) break;
								if (xb_2 <= xb2) continue;
								xb2 = xb_2;
								point_b = point;
								b_xb_size_new = true;
								//DrawDebugPixelPoint2(iPointB, RGB(0, 0, 255), 5);
							}

							for (auto xb_2 = xb1 - img_proc_xb_skip; xb_2 > xb_start; xb_2 -= img_proc_xb_skip)
							{
								point = xa_2 + xb_2;
								if (image_area_data[point / 4]) break;
								if (!is_image_area(point, 10)) break;
								if (xb_2 >= xb1) continue;
								xb1 = xb_2;
								point_a = point;
								b_xb_size_new = true;
								//DrawDebugPixelPoint2(iPointA, RGB(0, 0, 255), 5);
							}
						}

						if (!b_xb_size_new) break;


						auto is_xa_size_new = false;

						for (auto xb_2 = xb1; xb_2 <= xb2; xb_2 += img_proc_xb_skip)
						{
							for (auto xa_2 = xa2 + img_proc_xa_skip; xa_2 < xa_end; xa_2 += img_proc_xa_skip)
							{
								point = xa_2 + xb_2;
								if (image_area_data[point / 4]) break;
								if (!is_image_area(point, 10)) break;
								if (xa_2 <= xa2) continue;
								xa2 = xa_2;
								point_c = point;
								is_xa_size_new = true;
							}

							for (auto xa_2 = xa1 - img_proc_xa_skip; xa_2 > xa_start; xa_2 -= img_proc_xa_skip)
							{
								point = xa_2 + xb_2;
								if (image_area_data[point / 4]) break;
								if (!is_image_area(point, 10)) break;
								if (xa_2 >= xa1) continue;
								xa1 = xa_2;
								point_a = point;
								is_xa_size_new = true;
							}

							if (!is_xa_size_new) break;
						}
					}
#endif

#if 1 // Filter 2 - PROCESS_LAYER_IMPROVE_POINTS

					for (auto point = point_c + xb_size; point < xa_size; point += xb_size)
					{
						if (
							common_colors[(pixels[point + 2] + pixels[point + 1] + pixels[point]) / 3]
							||
							image_area_data[point / 4])
						{
							xa2 = GET_XA(point - xb_size);
							break;
						}
					}


					auto point_end = xa1 + xb_size0_b;
					for (auto point = point_b + 4; point < point_end; point += 4)
					{
						if (
							common_colors[(pixels[point + 2] + pixels[point + 1] + pixels[point]) / 3]
							||
							image_area_data[point / 4])
						{
							xb2 = GET_XB(point - 4);
							break;
						}
					}


					for (auto point = point_a - 4; point > xa1; point -= 4)
					{
						if (common_colors[(pixels[point + 2] + pixels[point + 1] + pixels[point]) / 3]
							||
							image_area_data[point / 4])
						{
							xb1 = GET_XB(point + 4);
							break;
						}
					}


					for (auto point = point_a - xb_size; point > 0; point -= xb_size)
					{
						if (
							common_colors[(pixels[point + 2] + pixels[point + 1] + pixels[point]) / 3]
							||
							image_area_data[point / 4])
						{
							xa1 = GET_XA(point + xb_size);
							break;
						}
					}

#endif

#if 1 // Filter 3 - PROCESS_LAYER_IMPROVE_BORDERS


					auto is_uniform_color_from_line_exists = [&](int point_a, int point_b, int skipLevel)
					{
						auto unique_pixels = 0;

						for (auto point = point_a + skipLevel; point <= point_b; point += skipLevel)
							if (!common_colors[(pixels[point + 2] + pixels[point + 1] + pixels[point]) / 3])
								unique_pixels++;
							else
								unique_pixels--;

						if (unique_pixels > 0)
							return false;
						else
							return true;
					};


					// Improve iXa1

					auto xa_2 = xa1 - xb_size;
					if (xa_2 > 0)
					{
						point_a = xa_2 + xb1;
						point_b = xa_2 + xb2;
						///wp(iPointA); wp(iPointB);
						if (!is_uniform_color_from_line_exists(point_a, point_b, 4))
						{
							///DrawDebugPixelPoint2r(iPointA, RGB(0, 255, 0), 10);
							for (xa_2 -= xb_size; xa_2 > 0; xa_2 -= xb_size)
								if (is_uniform_color_from_line_exists(xa_2 + xb1, xa_2 + xb2, 4))
								{
									xa1 = xa_2 + xb_size;
									break;
								}
						}
					}


					// Improve iXa2

					if (!is_uniform_color_from_line_exists(xa2 + xb1, xa2 + xb2, 4))
					{
						xa_2 = xa2 + xb_size;
						auto xa_max = xa_end - xb_size;
						if (xa_2 < xa_max)
						{
							if (!is_uniform_color_from_line_exists(xa_2 + xb1, xa_2 + xb2, 4))
							{
								for (xa_2 += xb_size; xa_2 < xa_max; xa_2 += xb_size)
									if (is_uniform_color_from_line_exists(xa_2 + xb1, xa_2 + xb2, 4))
									{
										xa2 = xa_2 - xb_size;
										break;
									}
							}
						}
					}
					else
					{
						for (xa_2 = xa2 - xb_size; xa_2 > xa1; xa_2 -= xb_size)
							if (!is_uniform_color_from_line_exists(xa_2 + xb1, xa_2 + xb2, 4))
							{
								xa2 = xa_2;
								break;
							}
					}


					int xb_2;
					if (!is_uniform_color_from_line_exists(xa1 + xb1, xa2 + xb1, xb_size))
					{
						xb_2 = xb1 - 4;
						if (xb_2 > 0)
						{
							if (!is_uniform_color_from_line_exists(xa1 + xb_2, xa2 + xb_2, xb_size))
							{
								for (xb_2 -= 4; xb_2 > 0; xb_2 -= 4)
									if (is_uniform_color_from_line_exists(xb_2 + xa1, xb_2 + xa2, xb_size))
									{
										xb1 = xb_2 + 4;
										break;
									}
							}
						}
					}
					else
					{
						for (auto xb_2 = xb1 + 4; xb_2 < xb2; xb_2 += 4)
							if (!is_uniform_color_from_line_exists(xa1 + xb_2, xa2 + xb_2, xb_size))
							{
								xb1 = xb_2;
								break;
							}
					}


					if (!is_uniform_color_from_line_exists(xb2 + xa1, xb2 + xa2, xb_size))
					{
						xb_2 = xb2 + 4;
						if (xb_2 < xb_size)
						{
							if (!is_uniform_color_from_line_exists(xb_2 + xa1, xb_2 + xa2, xb_size))
							{
								for (xb_2 += 4; xb_2 < xb_size; xb_2 += 4)
									if (is_uniform_color_from_line_exists(xb_2 + xa1, xb_2 + xa2, xb_size))
									{
										xb2 = xb_2 - 4;
										break;
									}
							}
						}
					}
					else
					{
						for (xb_2 = xb2 - 4; xb_2 > xb1; xb_2 -= 4)
							if (!is_uniform_color_from_line_exists(xb_2 + xa1, xb_2 + xa2, xb_size))
							{
								xb2 = xb_2;
								break;
							}
					}

#endif

					for (auto xa_2 = xa1; xa_2 <= xa2; xa_2 += xb_size)
						for (auto xb_2 = xb1; xb_2 <= xb2; xb_2 += 4)
						{
							auto point = xa_2 + xb_2;
							image_area_data[point / 4] = true;
						}
				}
			}


			return image_area_data;
		}
	}


	namespace glass_effect
	{
		bool is_enabled = false;
		double images_level, shapes_level, background_level;
		bool dark_background_mode = false;

		byte* pixels_reduced = nullptr;
		int x_size_reduced, y_size_reduced;
		int xy_size_reduced;


		constexpr int cube_size = 5;
		constexpr int cube_dim = cube_size * cube_size;

		void enable(const double glass_background, const bool glass_dark_background,
		            const double glass_images, const double glass_shapes)
		{
			is_enabled = true;
			glass_effect::background_level = glass_background;
			glass_effect::images_level = glass_images;
			glass_effect::shapes_level = glass_shapes;
			glass_effect::dark_background_mode = glass_dark_background;
		}

		void set_background_level(const double glass_background)
		{
			glass_effect::background_level = glass_background;
		}

		void set_shapes_level(const double glass_shapes)
		{
			glass_effect::shapes_level = glass_shapes;
		}

		void set_dark_background_mode(const bool enable)
		{
			glass_effect::dark_background_mode = enable;
		}

		void disable()
		{
			is_enabled = false;
		}

		// Unload the resources that used for the algorithem that detect each pixel that is text or image
		void free_resources()
		{
			// Allocate memory
			if (pixels_reduced)
			{
				delete pixels_reduced;
				pixels_reduced = nullptr;
			}
		}


		bool init()
		{
			free_resources();

			x_size_reduced = x_size / cube_size + 1;
			y_size_reduced = y_size / cube_size + 1;
			xy_size_reduced = x_size_reduced * y_size_reduced;

			pixels_reduced = pixels_reduced = new byte[xy_size_reduced];

			return true;
		}


		void map_shapes(double background)
		{
			// Build reduced map
			{
				for (auto y_r = 0; y_r < y_size_reduced; y_r++)
					for (auto x_r = 0; x_r < x_size_reduced; x_r++)
					{
						int colors[256] = {0};
						auto max_color_count = 1;
						const auto point_r = y_r * x_size_reduced + x_r;

						const auto y = y_r * cube_size;
						const auto x = x_r * cube_size;

						const auto point = y * x_size + x;
						byte max_color = (pixels[point] + pixels[point + 1] + pixels[point + 2]) / 3;


						auto y_max = y + cube_size;
						if (y_max > y_size) y_max = y_size;
						auto x_max = x + cube_size;
						if (x_max > x_size) x_max = x_size;


						for (auto y2 = y; y2 < y_max; y2++)
							for (auto x2 = x; x2 < x_max; x2++)
							{
								const auto xy_point = y2 * x_size + x2;
								if (map_images::image_area_data && map_images::image_area_data[xy_point]) continue;
								auto point = y2 * x_size * 4 + x2 * 4;
								const byte avg_color = (pixels[point] + pixels[point + 1] + pixels[point + 2]) / 3;
								colors[avg_color]++;
								if (colors[avg_color] > max_color_count)
								{
									max_color = avg_color;
									max_color_count = colors[avg_color];
								}
							}

						pixels_reduced[point_r] = max_color;
					}
			}

			// Reduce noise in the map
			{
				auto process = [&](unsigned int point, const int point_max, const int point_jump, const int max_count)
				{
					const int point_start = point;
					int point_end = point;
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


				for (auto y = 0; y < y_size_reduced - 1; y++)
				{
					auto point = y * x_size_reduced;
					const auto point_max = point + x_size_reduced - 1;
					while (point < point_max)
						point = process(point, point_max, 1, 5) + 1;
				}

				for (auto x = 0; x < x_size_reduced; x++)
				{
					auto point = x;
					auto point_max = x + (y_size_reduced - 1) * x_size_reduced;
					while (point < point_max)
						point = process(point, point_max, x_size_reduced, 4) + x_size_reduced;
				}
			}

#if 0 // Debug - pring reduced map
			for (auto y_r = 0; y_r < y_size_reduced; y_r++)
				for (auto x_r = 0; x_r < x_size_reduced; x_r++)
				{
					auto point_r = y_r * x_size_reduced + x_r;
					auto y = y_r * cube_size;
					auto x = x_r * cube_size;
					auto y_max = y + cube_size; if (y_max > y_end) y_max = y_end;
					auto x_max = x + cube_size; if (x_max > x_end) x_max = x_end;

					for (auto y2 = y; y2 < y_max; y2++)
						for (auto x2 = x; x2 < x_max; x2++)
						{
							auto point = y2 * x_end * 4 + x2 * 4;
							pixels[point] = pixels[point + 1] = pixels[point + 2] = pixels_reduced[point_r];
						}
				}


#endif

			// Mark shapes
			{
				for (auto y_r = 0; y_r < y_size_reduced; y_r++)
					for (auto x_r = 0; x_r < x_size_reduced; x_r++)
					{
						const auto point_r = y_r * x_size_reduced + x_r;
						const auto reduced_color = pixels_reduced[point_r];
						const auto y = y_r * cube_size;
						const auto x = x_r * cube_size;
						auto y_max = y + cube_size;
						if (y_max > y_size) y_max = y_size;
						auto x_max = x + cube_size;
						if (x_max > x_size) x_max = x_size;


						byte shape_max_brightness = 0;

						for (auto y2 = y; y2 < y_max; y2++)
							for (auto x2 = x; x2 < x_max; x2++)
							{
								const auto xy_point = y2 * x_size + x2;
								if (map_images::image_area_data && map_images::image_area_data[xy_point]) continue;
								const auto point = y2 * x_size * 4 + x2 * 4;

								const byte avg_color = (pixels[point] + pixels[point + 1] + pixels[point + 2]) / 3;

								const auto is_shape_color = avg_color != reduced_color;

								if (is_shape_color)
								{
									if (avg_color > shape_max_brightness)
										shape_max_brightness = avg_color;
								}
							}


						float scalar = 255.0 / static_cast<float>(shape_max_brightness);

						scalar *= shapes_level;
						

						for (auto y2 = y; y2 < y_max; y2++)
							for (auto x2 = x; x2 < x_max; x2++)
							{
								const auto xy_point = y2 * x_size + x2;
								if (map_images::image_area_data && map_images::image_area_data[xy_point]) continue;
								const auto point = y2 * x_size * 4 + x2 * 4;

								const auto is_shape_color = (pixels[point] + pixels[point + 1] + pixels[point + 2]) / 3
									!= reduced_color;


								if (is_shape_color)
								{

									if (scalar > 1)
									{

										int b = pixels[point];
										int g = pixels[point + 1];
										int r = pixels[point + 2];


										b *= scalar;
										g *= scalar;
										r *= scalar;

										auto max = r > g ? r : g;
										if (b > max) max = b;

										if (max > 255)
										{

											const auto reduce_scalar = 255 / static_cast<float>(max);
											b *= reduce_scalar;
											g *= reduce_scalar;
											r *= reduce_scalar;
										}

										pixels[point] = b;
										pixels[point + 1] = g;
										pixels[point + 2] = r;
									}
								}
								else
								{
									if (dark_background_mode)
									{
										if (reduced_color > 128)
										{
											pixels[point] = 255 - pixels[point];
											pixels[point + 1] = 255 - pixels[point + 1];
											pixels[point + 2] = 255 - pixels[point + 2];
										}
									}

									if (background_level != 1)
									{
										if (background_level != 0)
										{
											pixels[point] *= background_level;
											pixels[point + 1] *= background_level;
											pixels[point + 2] *= background_level;
											pixels[point + 3] *= background_level;
										}
										else
										{
											memset(&pixels[point], 0, sizeof(unsigned char) * 4);
										}
									}
								}
							}
					}
			}
		}
	}

	void set_default_settings()
	{
		map_images::is_enabled = false;
		glass_effect::is_enabled = false;
	}

	void free_resources()
	{
		map_images::free_resources();
		glass_effect::free_resources();

		if (cached_pixels)
		{
			free(cached_pixels);
			cached_pixels = nullptr;
		}

		x_size = y_size = 0;

		EmptyWorkingSet(GetCurrentProcess()); // Reduce memory usage
	}

	void enable_cache_buffer(const bool enable)
	{
		is_enable_cached_buffer = enable;
		if (!enable && cached_pixels)
		{
			free(cached_pixels);
			cached_pixels = nullptr;
		}
	}

	void init(ID3D11DeviceContext* d3d_context)
	{
		process_layer_cpu::d3d_context = d3d_context;
	}

	// Free resources that used by this object. This method called also when you use `delete` keyword
	bool load_frame(byte* pixels, int x_size, int y_size, int x_end,
	                int y_end)
	{
		process_layer_cpu::pixels = pixels;

		if (process_layer_cpu::x_size != x_size || process_layer_cpu::y_size != y_size)
		{
			process_layer_cpu::x_size = x_size;
			process_layer_cpu::y_size = y_size;
			process_layer_cpu::x_end = x_end ? x_end : x_size;
			process_layer_cpu::y_end = y_end ? y_end : y_size;

			xb_size = x_size * 4;
			xa_size = (y_size - 1) * xb_size;
			xb_size0_b = xb_size - 4;
			xab_size = x_size * (y_size - 1) * 4;

			xy_size = x_size * (y_size + 1);

			xa_start = xb_size * 4;
			xa_end = xa_size - xb_size * 4;
			xb_start = 4 * 8;
			xb_end = xb_size - 4 * 8;

			if (cached_pixels)
			{
				free(cached_pixels);
				cached_pixels = nullptr;
			}

			if (is_enable_cached_buffer)
			{
				cached_pixels = static_cast<byte*>(malloc(x_size * 4 * y_size * sizeof(byte)));
				if (!cached_pixels)
				{
					std::cout << "Failed to allocate memory for cached_pixels\n";
					return false;
				}

				memcpy(cached_pixels, pixels, x_size * 4 * y_size * sizeof(byte));
			}

			map_images::free_resources();
			glass_effect::free_resources();

			if (map_images::is_enabled && !map_images::init())
			{
				std::cout << "map_images::init() failed\n";
				return false;
			}

			if (glass_effect::is_enabled && !glass_effect::init())
			{
				std::cout << "glass_effect::init() failed\n";
				return false;
			}

			EmptyWorkingSet(GetCurrentProcess());
		}

		return true;
	}


	bool is_new_pixels()
	{
		auto same_pixels = true;
		for (auto y = 0; y < y_end; y++)
		{
			const unsigned int offset = y * x_size * 4;
			if (memcmp(cached_pixels + offset, pixels + offset, (x_end - 1) * 4 * sizeof(byte)) != 0)
			{
				same_pixels = false;
				break;
			}
		}

		if (same_pixels)
			return false;

		memcpy(cached_pixels, pixels, xab_size);
		return true;
	}

	bool begin_process(ID3D11Texture2D* texture, const int x_end, const int y_end)
	{
		// map the texture
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
			std::cout << "Failed to get mapped cpu texture\n";
			return false;
		}

		process_layer_cpu::texture = texture;
		return load_frame(static_cast<byte*>(map_info.pData), map_info.RowPitch / 4,
		                  map_info.DepthPitch / map_info.RowPitch,
		                  x_end, y_end);
	}


	void end_process()
	{
		if (texture)
		{
			d3d_context->Unmap(texture, 0);
			texture = nullptr;
		}
	}

	void invert_colors()
	{
		if (!map_images::image_area_data)
		{
			// Code to process the pixels goes here
			for (auto y = 0; y < y_size; y++)
				for (auto x = 0; x < x_size; x++)
				{
					const unsigned int point = y * x_size * 4 + x * 4;
					pixels[point] = ~pixels[point];
					pixels[point + 1] = ~pixels[point + 1];
					pixels[point + 2] = ~pixels[point + 2];
				}
		}
		else
		{
			for (auto y = 0; y < y_size; y++)
				for (auto x = 0; x < x_size; x++)
				{
					unsigned int point = y * x_size + x;
					if (map_images::image_area_data[point]) continue;
					point *= 4;
					pixels[point] = ~pixels[point];
					pixels[point + 1] = ~pixels[point + 1];
					pixels[point + 2] = ~pixels[point + 2];
				}
		}
	}


	bool is_pixels_bright(byte* cpu_texture_pixels, const int x_size, const int y_size)
	{
		const auto pixels_skip = 30;
		auto total_points = 0;
		auto bright_points = 0;
		if (!map_images::image_area_data)
		{
			for (auto y = 0; y < y_size; y += pixels_skip)
				for (auto x = 0; x < x_size; x += pixels_skip)
				{
					const unsigned int point = y * x_size * 4 + x * 4;
					total_points++;
					if ((cpu_texture_pixels[point] + cpu_texture_pixels[point + 1] + cpu_texture_pixels[point + 2]) / 3
						> 127)
						bright_points++;
				}
		}
		else
		{
			for (auto y = 0; y < y_size; y += pixels_skip)
				for (auto x = 0; x < x_size; x += pixels_skip)
				{
					unsigned int point = y * x_size + x;
					if (map_images::image_area_data[point]) continue;
					total_points++;
					point *= 4;
					if ((cpu_texture_pixels[point] + cpu_texture_pixels[point + 1] + cpu_texture_pixels[point + 2]) / 3
						> 127)
						bright_points++;
				}
		}

		return bright_points / static_cast<double>(total_points) > 0.5;
	}

	bool is_current_pixels_bright()
	{
		return is_pixels_bright(pixels, x_size, y_size);
	}
}
