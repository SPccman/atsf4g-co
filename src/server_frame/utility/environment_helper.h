#ifndef _UTILITY_ENVIRONMENT_HELPER_H_
#define _UTILITY_ENVIRONMENT_HELPER_H_

#pragma once

#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1600)
#include <unordered_map>
#define UTIL_ENV_AUTO_MAP(...) std::unordered_map<__VA_ARGS__>

#else
#include <map>
#define UTIL_ENV_AUTO_MAP(...) std::map<__VA_ARGS__>
#endif

#endif