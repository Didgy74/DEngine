//#include "SDL2/SDL.h"

#include "Engine/Engine.hpp"

#include <exception>
#include <iostream>

int main(int argc, char* argv[])
{
	try
	{
		Engine::Core::Run();
	}
	catch (std::exception e)
	{
		std::cout << e.what() << std::endl;
	}


    return 0;
}