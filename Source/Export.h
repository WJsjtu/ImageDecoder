#pragma once

#pragma warning(disable : 4251)
#pragma warning(disable : 4275)

#ifdef _WIN32
#ifdef IMAGE_DLL
#define IMAGE_PORT __declspec(dllexport)
#else
#define IMAGE_PORT __declspec(dllimport)
#endif
#else
#define IMAGE_PORT
#endif