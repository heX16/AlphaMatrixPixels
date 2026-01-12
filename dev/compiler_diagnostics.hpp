// Compiler version detection and diagnostics
// Add this at the top of fixed_point.hpp or in a separate header

// Check if specific features are available
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304L
  #warning CONSTEXPR_AVAILABLE_CXX14
#elif defined(__cpp_constexpr) && __cpp_constexpr >= 200704L
  #warning CONSTEXPR_AVAILABLE_CXX11
#else
  #warning CONSTEXPR_NOT_AVAILABLE
#endif

#if defined(__cpp_if_constexpr) && __cpp_if_constexpr >= 201606L
  #warning IF_CONSTEXPR_AVAILABLE
#else
  #warning IF_CONSTEXPR_NOT_AVAILABLE
#endif

#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
  #warning INLINE_VARIABLES_AVAILABLE
#else
  #warning INLINE_VARIABLES_NOT_AVAILABLE
#endif

// Check __cpp_constexpr value directly
#ifdef __cpp_constexpr
  #if __cpp_constexpr >= 201603L
    #warning CONSTEXPR_LEVEL: C++17_style
  #elif __cpp_constexpr >= 201304L
    #warning CONSTEXPR_LEVEL: C++14_style
  #elif __cpp_constexpr >= 200704L
    #warning CONSTEXPR_LEVEL: C++11_style
  #else
    #warning CONSTEXPR_LEVEL: unknown
  #endif
#endif

// C++ standard feature detection
#ifdef __cpp_constexpr
  #warning CPP_CONSTEXPR: __cpp_constexpr
#else
  #warning CPP_CONSTEXPR_NOT_DEFINED
#endif

#ifdef __cpp_if_constexpr
  #warning CPP_IF_CONSTEXPR: __cpp_if_constexpr
#else
  #warning CPP_IF_CONSTEXPR_NOT_DEFINED
#endif


// C++ standard detection (numeric values)
#if __cplusplus >= 201703L
  #warning CXX_STANDARD: C++17_or_later
#elif __cplusplus >= 201402L
  #warning CXX_STANDARD: C++14
#elif __cplusplus >= 201103L
  #warning CXX_STANDARD: C++11
#else
  #warning CXX_STANDARD: C++98_or_earlier
#endif
