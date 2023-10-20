#ifndef J2534OVERIP_GLOBAL_H
#define J2534OVERIP_GLOBAL_H

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define Q_DECL_EXPORT __declspec(dllexport)
#  define Q_DECL_IMPORT __declspec(dllimport)
#else
#  define Q_DECL_EXPORT     __attribute__((visibility("default")))
#  define Q_DECL_IMPORT     __attribute__((visibility("default")))
#endif

#if defined(J2534OVERIP_LIBRARY)
#  define J2534OVERIP_EXPORT Q_DECL_EXPORT
#  define J2534_EXPORT int32_t __stdcall
#else
#  define J2534OVERIP_EXPORT Q_DECL_IMPORT
#endif

#define WIN32_LEAN_AND_MEAN

//#define EXPORT __declspec(dllexport) int32_t __stdcall

/*
class J2534OVERIP_EXPORT j2534
{
public:
    j2534();
};
*/

#endif // J2534OVERIP_GLOBAL_H
