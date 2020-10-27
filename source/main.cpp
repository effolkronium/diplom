#include "RenderOpengl.h"
#include "RenderVulkan.h"
#include "Scene.h"

#include <iostream>
#include <string>
#include <filesystem>

using namespace std::literals;
namespace fs = std::filesystem;

#ifdef _WIN32

#include "Windows.h"

namespace
{
	void SetupCurrentDirectory()
	{
		size_t length = MAX_PATH;
		std::wstring modulePath;

		while (true)
		{
			modulePath.resize(length);
			DWORD size = GetModuleFileName(NULL, &modulePath[0], MAX_PATH);

			DWORD err = GetLastError();

			if (err == ERROR_SUCCESS)
				break;

			if (err == ERROR_INSUFFICIENT_BUFFER)
				length *= 2;
			else
				throw std::runtime_error{ "GetModuleFileName error: "s + std::to_string(err) };
		}

		if (0 == SetCurrentDirectory(fs::path{ modulePath }.parent_path().c_str()))
			throw std::runtime_error{ "SetCurrentDirectory error: "s + std::to_string(GetLastError()) };
	}
#endif
}

#include <thread>

int main()
try {
#ifdef _WIN32
	SetupCurrentDirectory();
#endif

	
	RenderVulkan openglRender;
	Scene scene{ openglRender };

	scene.run();
	
	/*RenderOpengl opengl;
	opengl.startRenderLoop();*/
}
catch (const std::exception& ex)
{
	std::cerr << "Main exception: " << ex.what() << std::endl;
	std::cin.get();
}
catch (...)
{
	std::cerr << "\n\nUNSPESIFIED EXCEPTION!!!\n\n";
	std::cin.get();
}