#include <Windows.h>
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#include <plog/Log.h>

#include "capture_layer_bitblt.h"

#include <iostream>

// Capture layer using the Magnification API
namespace capture_layer_bitblt
{
	int x_size = 0, y_size = 0;

	BITMAPINFO bitmap_info;
	HBITMAP bitmap_src = nullptr;
	HGDIOBJ old_object;

	HWND screen_hwnd = nullptr;
	HDC hdc_display = nullptr;
	HDC hdc_rect = nullptr;

	void un_init_capture()
	{
		if (bitmap_src)
		{
			SelectObject(hdc_rect, old_object);
			DeleteObject(bitmap_src);
			bitmap_src = nullptr;
		}

		pixels = nullptr;
		x_size = y_size = 0;
	}

	bool create_capture_buffer(const int x_size, const int y_size)
	{
		std::cout << "Creating capture buffer" << std::endl;

		if (bitmap_src)
		{
			SelectObject(hdc_rect, old_object);
			DeleteObject(bitmap_src);
		}

		screen_hwnd = GetDesktopWindow();
		hdc_display = GetDC(screen_hwnd);
		hdc_rect = CreateCompatibleDC(hdc_display); // Create compatible DC 


		ZeroMemory(&bitmap_info, sizeof(BITMAPINFO));
		bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmap_info.bmiHeader.biWidth = x_size;
		bitmap_info.bmiHeader.biHeight = -y_size; //negative so (0,0) is at top left
		bitmap_info.bmiHeader.biPlanes = 1;
		bitmap_info.bmiHeader.biBitCount = 32;
		bitmap_info.bmiHeader.biCompression = BI_RGB;

		bitmap_src = CreateDIBSection(hdc_display, &bitmap_info, DIB_RGB_COLORS, reinterpret_cast<void**>(&pixels),
		                              nullptr, 0);

		old_object = SelectObject(hdc_rect, bitmap_src);
		if (!old_object)
		{
			std::cout << "Failed to create capture buffer" << std::endl;
			un_init_capture();
			return false;
		}

		return true;
	}


	bool capture(const int x_pos, const int y_pos, const int x_size, const int y_size)
	{
		if (capture_layer_bitblt::x_size != x_size || capture_layer_bitblt::y_size != y_size)
		{
			if (!create_capture_buffer(x_size, y_size))
				return false;

			capture_layer_bitblt::x_size = x_size;
			capture_layer_bitblt::y_size = y_size;
		}


		// Init if needed
		BitBlt(hdc_rect, 0, 0, x_size, y_size, hdc_display, x_pos, y_pos, SRCCOPY);

		return true;
	}
}
