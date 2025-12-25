#pragma once

/*
Compile-time keyword hook for FUNCTIONS.

Default: AMP_CONSTEXPR = constexpr.

Why this exists:
 - Some older/embedded C++ toolchains (Arduino-like) have incomplete/buggy support for `constexpr`
   in functions (especially when the function body uses float math or float->int conversions).
 - On such compilers this may compile very slowly, produce strange errors, or fail to compile at all.

If you hit compilation problems, you can switch constexpr-off for functions by overriding the macro
 (e.g. via compiler flags or before including this header):
   #define AMP_CONSTEXPR inline

Note:
 - This macro must be used ONLY for functions (not for variables/constants).
 - Replacing constexpr with inline means the function is no longer a compile-time constant expression.
*/
#ifndef AMP_CONSTEXPR
#  if defined(ARDUINO)
// Many Arduino toolchains are stuck on older GCC versions where `constexpr` can be buggy/slow
// (especially with float math and some union/aggregate patterns). Default to `inline` there.
#    define AMP_CONSTEXPR inline
#  else
#    define AMP_CONSTEXPR constexpr
#  endif
#endif

