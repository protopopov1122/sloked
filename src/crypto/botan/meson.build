botan_crypto_src = [
  'Botan.cpp'
]

botan = dependency('botan-2')
sloked_botan_crypto_lib = static_library('sloked_crypto', botan_crypto_src,
  include_directories : include_directories('../../../headers/crypto/botan'),
  dependencies : [botan, sloked_framework])
sloked_botan_crypto = declare_dependency(link_with : sloked_botan_crypto_lib,
  include_directories : include_directories('../../../headers/crypto/botan'))