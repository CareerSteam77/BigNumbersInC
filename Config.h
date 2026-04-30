#pragma once

//PLATFORM DETECTION
#if   defined(_WIN32)      || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__APPLE__)   || defined(__MACH__)
    #define PLATFORM_APPLE
#elif defined(__linux__)
    #define PLATFORM_LINUX
#else
    #error "Platform cannot be detected. Please configure manually."
#endif


// HARDWARE DETECTION

// SUPPORT FOR 128 BIT
#if defined(__SIZEOF_INT128__)
    #define SUPPORT_UINT128 1
    #define MAX_UINT128 (~((__uint128_t)0))
#else
    #define SUPPORT_UINT128 0
#endif

// SUPPORT FOR MULTITHREADING
#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_APPLE) || defined(PLATFORM_LINUX)
    #define SUPPORT_MULTITHREADING 1
#else
    #define SUPPORT_MULTITHREADING 0
#endif