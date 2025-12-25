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
amp_inc = include_directories('../../src')

# System libraries for Windows/MinGW
cc = meson.get_compiler('cpp')

# SDL2 include directories - compiler should find them automatically
# but we can add them explicitly if needed
sdl2_inc = []

# Manual library linking - same approach as tasks.json
# This allows -static to work properly
sdl2_lib = cc.find_library('SDL2', required : true)
sdl2main_lib = cc.find_library('SDL2main', required : true)
sdl2_ttf_lib = cc.find_library('SDL2_ttf', required : true)

# MinGW runtime and Windows system libraries
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
  # Full static linking - exactly like tasks.json
  static_link_args += ['-static', '-static-libstdc++', '-static-libgcc']
endif

# Source files
gui_test_sources = files('main.cpp')

# Executable
# Manual linking with -static flag - exactly like tasks.json
# All libraries in dependencies, -static in link_args forces static linking
gui_test_exe = executable('HXLED_GUI_Test',
  gui_test_sources,
  include_directories : [amp_inc] + sdl2_inc,
  dependencies : [
    sdl2main_lib,
    sdl2_lib,
    sdl2_ttf_lib
  ] + windows_libs + sdl2_ttf_deps,
  link_args : ['-static', '-static-libstdc++', '-static-libgcc'],
  cpp_args : ['-m64', '-Wall'],
  install : false
)

