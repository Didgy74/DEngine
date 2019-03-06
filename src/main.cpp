#include "SDL2/SDL.h"

#include "Engine/Engine.hpp"

#include <functional>

struct Test
{
    ~Test()
    {

    }
};

template<typename T>
void Yo(void* ptr)
{
    delete static_cast<T*>(ptr);
}

int main(int argc, char* argv[])
{
    Engine::Core::Run();

    std::function<void(void*)> deleter = Yo<Test>;

    deleter(new Test());

    return 0;
}