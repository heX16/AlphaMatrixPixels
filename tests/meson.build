# Tests executable
project('Tests', 'cpp',
  version : '1.0.0',
  default_options : [
    'cpp_std=c++17',
    'warning_level=2',
    'default_library=static'
  ]
)

# Path to library source (relative to this meson.build location)
# This meson.build is in tests/, so library is at ../src/
amp_inc = include_directories('../src')

# Source files
test_sources = files('pixel_matrix_tests.cpp')

# Executable
pixel_matrix_tests_exe = executable('pixel_matrix_tests',
  test_sources,
  include_directories : amp_inc,
  link_args : ['-static', '-static-libstdc++', '-static-libgcc'],
  cpp_args : ['-Wall'],
  install : false
)

# Run tests
test('pixel_matrix_tests', pixel_matrix_tests_exe)
