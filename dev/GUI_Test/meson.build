# GUI_Test executable - standalone project
project('GUI_Test', 'cpp',
  version : '1.0.0',
  default_options : [
    'cpp_std=c++17',
    'warning_level=2',
    'default_library=static'
  ]
)

# Path to library source (relative to this meson.build location)
# This meson.build is in dev/GUI_Test/, so library is at ../../src/
alpha_matrix_pixels_inc = include_directories('../../src')

# System libraries for Windows/MinGW
cc = meson.get_compiler('cpp')

# SDL2 include directories - compiler should find them automatically
# but we can add them explicitly if needed
sdl2_inc = []

# Manual library linking - same approach as tasks.json
# This allows -static to work properly
sdl2_lib = cc.find_library('SDL2', required : true)
# SDL2main is Windows-specific (provides WinMain entry point)
sdl2main_lib = cc.find_library('SDL2main', required : host_machine.system() == 'windows')
sdl2_ttf_lib = cc.find_library('SDL2_ttf', required : true)

# SDL2 libraries list (include SDL2main only on Windows)
sdl2_libs = [sdl2_lib, sdl2_ttf_lib]
if host_machine.system() == 'windows'
  sdl2_libs += [sdl2main_lib]
endif

# MinGW runtime and Windows system libraries (Windows only)
windows_libs = []
if host_machine.system() == 'windows'
  windows_libs = [
    cc.find_library('mingw32', required : true),
    cc.find_library('gdi32', required : false),
    cc.find_library('setupapi', required : false),
    cc.find_library('winmm', required : false),
    cc.find_library('imm32', required : false),
    cc.find_library('ole32', required : false),
    cc.find_library('oleaut32', required : false),
    cc.find_library('rpcrt4', required : false),
    cc.find_library('dwrite', required : false),
    cc.find_library('version', required : false),
  ]
endif

# SDL2_ttf dependencies (usually auto-linked, but explicit for clarity)
sdl2_ttf_deps = [
  cc.find_library('freetype', required : false),
  cc.find_library('harfbuzz', required : false),
  cc.find_library('graphite2', required : false),
  cc.find_library('png', required : false),
  cc.find_library('bz2', required : false),
  cc.find_library('brotlidec', required : false),
  cc.find_library('brotlicommon', required : false),
  cc.find_library('z', required : false),
]

# Static linking flags - same as in tasks.json
static_link_args = []
if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  static_link_args += [
    '-static',  # Static linking for all libraries
    '-static-libstdc++',  # Static linking for C++ standard library
    '-static-libgcc',  # Static linking for GCC runtime
  ]
endif

# Source files
gui_test_sources = files('main.cpp')

# Executable
# Manual linking with -static flag - exactly like tasks.json
# All libraries in dependencies, -static in link_args forces static linking
gui_test_exe = executable('HXLED_GUI_Test',
  gui_test_sources,
  # Header search paths for #include
  include_directories : [alpha_matrix_pixels_inc] + sdl2_inc,
  # Libraries to link against (SDL2, Windows system libs, etc.)
  dependencies : sdl2_libs  # SDL2 libraries (SDL2main included on Windows only)
    + windows_libs  # Windows system libraries (mingw32, gdi32, winmm, etc.)
    + sdl2_ttf_deps,  # SDL2_ttf dependencies (freetype, harfbuzz, png, etc.)
  # Linker flags (static linking)
  link_args : static_link_args,
  # Compiler flags for C++ compilation
  cpp_args : [
    '-m64',  # 64-bit architecture
    '-Wall',  # Enable all warnings
    '-Wno-unused-variable',  # Suppress unused variable warnings
    '-Wno-unused-parameter',  # Suppress unused parameter warnings
  ],
  # Don't install executable (build only)
  install : false
)

