openssl_crypto_src = [
  'src/OpenSSL.cpp'
]

openssl = dependency('openssl')
sloked_openssl_crypto_flags = ['-DSLOKED_FEATURE_CRYPTO', '-DSLOKED_FEATURE_CRYPTO_OPENSSL']
sloked_openssl_crypto_lib = static_library('sloked_crypto_openssl', openssl_crypto_src,
  cpp_args : sloked_openssl_crypto_flags,
  include_directories : include_directories('headers'),
  dependencies : [openssl, sloked_framework])
sloked_openssl_crypto = declare_dependency(link_with : sloked_openssl_crypto_lib,
  include_directories : include_directories('headers'))