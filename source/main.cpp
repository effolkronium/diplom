#include "RenderOpengl.h"
#include <iostream>
#include "Windows.h"

int main()
try {
	const char* a = "ab";
	const char* const b = "ba";
	a = b;

	
	RenderOpengl opengl;

	opengl.startRenderLoop();
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