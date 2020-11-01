#include "RenderOpengl.h"
#include "RenderVulkan.h"
#include "RenderGUI.h"
#include "Scene.h"

#include <iostream>
#include <string>
#include <filesystem>

using namespace std::literals;
namespace fs = std::filesystem;

#ifdef __linux__

#include <libgen.h>
#include <unistd.h>
#include <linux/limits.h>

namespace
{
        void SetupCurrentDirectory()
        {
                std::string modulePath;
                modulePath.resize(PATH_MAX);

                ssize_t count = readlink("/proc/self/exe", &modulePath[0], PATH_MAX);
                if(count < 0)
                        throw std::runtime_error("readlink failed"s + std::to_string(errno));

                char* path = dirname(&modulePath[0]);
				chdir(path);
        }
}
#endif

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
}
#endif

#include <thread>

#ifdef _WIN32
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#else
int main()
#endif
try {
	SetupCurrentDirectory();

	RenderGuiData guiData{};
	while (true)
	{
		RenderGUI gui;
		guiData = gui.startRenderLoop(guiData);

		if (guiData.renderType == RenderGuiData::RenderType::OpenGL)
		{
			RenderOpengl openglRender;
			Scene scene{ openglRender, guiData };
			guiData.averageFps = scene.run();
		}
		else if (guiData.renderType == RenderGuiData::RenderType::Vulkan)
		{
			RenderVulkan vulkanRender;
			Scene scene{ vulkanRender, guiData };
			guiData.averageFps = scene.run();
		}
		else
		{
			break;
		}
	}
}
catch (const std::exception& ex)
{
	auto wtf = ex.what();
	std::cerr << "Main exception: " << ex.what() << std::endl;
	std::cin.get();
}
catch (...)
{
	std::cerr << "\n\nUNSPESIFIED EXCEPTION!!!\n\n";
	std::cin.get();
}
