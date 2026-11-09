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

dependencies = [
]

lib_args = ['-DBUILDING_VEB']

sources = [
  'veb/veb.cpp',
]

inc_dirs = include_directories('veb')

lib = library(
  'veb',
  sources,
  install : true,
  include_directories : inc_dirs,
  cpp_shared_args: lib_args,
  gnu_symbol_visibility : 'hidden',
  dependencies: dependencies,
)

veb_dep = declare_dependency(
  include_directories : inc_dirs,
  dependencies : dependencies,
  link_with : lib,
)
meson.override_dependency('veb', veb_dep)

catch2_dep = dependency(
  'catch2-with-main',
  fallback : ['catch2', 'catch2_with_main_dep'],
  default_options: ['tests=false'],
)

test_dependencies = [
  catch2_dep,
  veb_dep,
]

test_exe = executable(
  'veb_test',
  'test/veb_test.cpp',
  dependencies : dependencies + test_dependencies,
  link_with : lib,
)
test('eda_structs', test_exe)

experiment_dependencies = [
  veb_dep,
]

dijkstra_exe = executable(
  'dijkstra',
  'experiments/dijkstra.cpp',
  dependencies : dependencies + experiment_dependencies,
  link_with : lib,
)

prim_exe = executable(
  'prim',
  'experiments/prim.cpp',
  dependencies : dependencies + experiment_dependencies,
  link_with : lib,
)

huffman_exe = executable(
  'huffman',
  'experiments/huffman.cpp',
  dependencies : dependencies + experiment_dependencies,
  link_with : lib,
)
