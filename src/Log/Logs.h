#pragma once
#include <cstdio>
#include <cstdarg>
#include <windows.h>


inline void LogPrint(const char* fmt, ...)
{
    char buffer[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);

    fprintf(stderr, "%s", buffer);
#ifdef _WIN32
    OutputDebugStringA(buffer);
#endif
}

#define LOG_INFO(fmt, ...)  do{ LogPrint("[INFO] " fmt "\n" __VA_OPT__(,) __VA_ARGS__); }while(0)
#define LOG_WARN(fmt, ...)  do{ LogPrint("[WARN] " fmt "\n" __VA_OPT__(,) __VA_ARGS__); }while(0)
#define LOG_ERROR(fmt, ...) do{ LogPrint("[ERROR] %s:%d " fmt "\n", __FILE__, __LINE__ __VA_OPT__(,) __VA_ARGS__); }while(0)


