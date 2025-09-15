
#pragma once

#ifndef USTC_CG_NAMESPACE_OPEN_SCOPE
#define USTC_CG_NAMESPACE_OPEN_SCOPE  namespace USTC_CG {
#define USTC_CG_NAMESPACE_CLOSE_SCOPE }
#endif

#if defined(_MSC_VER)
#define WEB_SERVER_EXPORT   __declspec(dllexport)
#define WEB_SERVER_IMPORT   __declspec(dllimport)
#define WEB_SERVER_NOINLINE __declspec(noinline)
#define WEB_SERVER_INLINE   __forceinline
#else
#define WEB_SERVER_EXPORT __attribute__((visibility("default")))
#define WEB_SERVER_IMPORT
#define WEB_SERVER_NOINLINE __attribute__((noinline))
#define WEB_SERVER_INLINE   __attribute__((always_inline)) inline
#endif

#if BUILD_WEB_SERVER_MODULE
#define WEB_SERVER_API    WEB_SERVER_EXPORT
#define WEB_SERVER_EXTERN extern
#else
#define WEB_SERVER_API WEB_SERVER_IMPORT
#if defined(_MSC_VER)
#define WEB_SERVER_EXTERN
#else
#define WEB_SERVER_EXTERN extern
#endif
#endif
