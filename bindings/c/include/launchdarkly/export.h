#pragma once

#ifdef _WIN32
#define LD_EXPORT(x) __declspec(dllexport) x
#else
#define LD_EXPORT(x) __attribute__((visibility("default"))) x
#endif
