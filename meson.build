project(
  'veb',
  'cpp',
  version : '0.1',
  meson_version : '>= 1.3.0',
  default_options : [
    'warning_level=3',
    'cpp_std=c++23',
    'b_sanitize=address,undefined',
    'b_lundef=false',
  ],
)

inc_dirs = include_directories('include')

dependencies = [
]

sources = [
  'src/veb.cpp',
]

exe = executable(
  'veb',
  sources,
  install : true,
  dependencies : dependencies,
  include_directories : inc_dirs,
)
