#include "DEngine/Utility.hpp"

#include <random>

template<>
DEngine::f32 DEngine::Std::Rand<DEngine::f32>()
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<f32> dis(0.f, 1.f);

    return dis(gen);
}

template<>
DEngine::u64 DEngine::Std::RandRange(u64 a, u64 b)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<u64> dis(a, b);

    return dis(gen);
}

template<>
DEngine::f32 DEngine::Std::RandRange(f32 a, f32 b)
{
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<f32> dis(a, b);

    return dis(gen);
}