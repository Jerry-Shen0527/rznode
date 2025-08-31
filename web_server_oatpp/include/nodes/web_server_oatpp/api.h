
#pragma once

#ifndef USTC_CG_NAMESPACE_OPEN_SCOPE
#define USTC_CG_NAMESPACE_OPEN_SCOPE  namespace USTC_CG {
#define USTC_CG_NAMESPACE_CLOSE_SCOPE }
#endif

#if defined(_MSC_VER)
#define WEB_SERVER_OATPP_EXPORT   __declspec(dllexport)
#define WEB_SERVER_OATPP_IMPORT   __declspec(dllimport)
#define WEB_SERVER_OATPP_NOINLINE __declspec(noinline)
#define WEB_SERVER_OATPP_INLINE   __forceinline
#else
#define WEB_SERVER_OATPP_EXPORT __attribute__((visibility("default")))
#define WEB_SERVER_OATPP_IMPORT
#define WEB_SERVER_OATPP_NOINLINE __attribute__((noinline))
#define WEB_SERVER_OATPP_INLINE   __attribute__((always_inline)) inline
#endif

#if BUILD_WEB_SERVER_OATPP_MODULE
#define WEB_SERVER_OATPP_API    WEB_SERVER_OATPP_EXPORT
#define WEB_SERVER_OATPP_EXTERN extern
#else
#define WEB_SERVER_OATPP_API WEB_SERVER_OATPP_IMPORT
#if defined(_MSC_VER)
#define WEB_SERVER_OATPP_EXTERN
#else
#define WEB_SERVER_OATPP_EXTERN extern
#endif
#endif
