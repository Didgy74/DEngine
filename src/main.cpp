//#include "SDL2/SDL.h"

//#include "Engine/Engine.hpp"

#include "Engine/AssetManager/TextureDocument.hpp"

int main(int argc, char* argv[])
{
	auto test = Engine::AssMan::LoadTextureDocument("Data/Textures/test.ktx");

    return 0;
}