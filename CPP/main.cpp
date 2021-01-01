
#include <iostream>
#include <Windows.h>
#include "WTRenderer/renderer.h"

int main()
{

    // std::cout << "test";

	renderer::init(true);
	
    auto* const target = reinterpret_cast<HWND>(0x0000000000100F7C);

    renderer::set_target(target);

    renderer::enable_glass_mode(true);

	while (true)
	{
		renderer::process_loop();
		
		MSG msg = { nullptr };

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			Sleep(10);
		}
	}
	
    return 0;
}
