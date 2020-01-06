botan_crypto_src = [
  'src/Botan.cpp'
]

botan = dependency('botan-2')
sloked_botan_crypto_flags = ['-DSLOKED_FEATURE_CRYPTO', '-DSLOKED_FEATURE_CRYPTO_BOTAN']
sloked_botan_crypto_lib = static_library('sloked_crypto_botan', botan_crypto_src,
  cpp_args : sloked_botan_crypto_flags,
  include_directories : include_directories('headers'),
  dependencies : [botan, sloked_framework])
sloked_botan_crypto = declare_dependency(link_with : sloked_botan_crypto_lib,
  include_directories : include_directories('headers'))