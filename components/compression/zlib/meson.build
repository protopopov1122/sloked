zlib_compression_src = [
  'src/Zlib.cpp'
]


zlib = dependency('zlib')
sloked_zlib_compression_flags = ['-DSLOKED_FEATURE_COMPRESSION', '-DSLOKED_FEATURE_COMPRESSION_ZLIB']
sloked_zlib_compression_lib = static_library('sloked_compression_zlib', zlib_compression_src,
  cpp_args : sloked_zlib_compression_flags,
  include_directories : include_directories('headers'),
  dependencies : [zlib, sloked_framework])
sloked_zlib_compression = declare_dependency(link_with : sloked_zlib_compression_lib,
  include_directories : include_directories('headers'))