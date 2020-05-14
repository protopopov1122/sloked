project('sloked', 'cpp', 'c', default_options : ['cpp_std=c++17', 'warning_level=3'])

compiler_id = meson.get_compiler('cpp').get_id()
if compiler_id == 'gcc' or compiler_id == 'clang'
  add_project_arguments('-Wno-unused-parameter', '-pedantic', language : 'cpp')
  add_project_link_arguments('-rdynamic', language : 'cpp')
elif compiler_id == 'pgi'
  add_project_arguments('-noswitcherror', language : 'cpp')
  add_project_link_arguments('-noswitcherror', '-rdynamic', language : 'cpp')
endif

sloked_disable_crypto = true
sloked_disable_compression = true
sloked_disable_scripting = true
sloked_disable_graphics = true

subdir('components/framework')

if get_option('crypto') == 'botan'
  subdir('components/crypto/botan')
  sloked_crypto = sloked_botan_crypto
  sloked_crypto_flags = sloked_botan_crypto_flags
  sloked_disable_crypto = false
elif get_option('crypto') == 'openssl'
  subdir('components/crypto/openssl')
  sloked_crypto = sloked_openssl_crypto
  sloked_crypto_flags = sloked_openssl_crypto_flags
  sloked_disable_crypto = false
endif
if get_option('compression') == 'zlib'
  subdir('components/compression/zlib')
  sloked_compression = sloked_zlib_compression
  sloked_compression_flags = sloked_zlib_compression_flags
  sloked_disable_compression = false
endif
subdir('components/platform/posix')
if get_option('scripting_lua')
  subdir('components/script/lua')
  sloked_scripting = sloked_lua_bindings
  sloked_scripting_flags = sloked_lua_bindings_flags
  sloked_disable_scripting = false
  subdir('components/lua-connector')
endif
if get_option('graphics') == 'cairo_sdl'
  subdir('components/ui-cairo')
  sloked_graphics = sloked_graphics_cairo_sdl
  sloked_graphics_flags = sloked_graphics_cairo_sdl_flags
  sloked_disable_graphics = false
endif
if get_option('build_headless')
  subdir('components/headless/application-lib')
endif


sloked_platform = sloked_posix_platform
sloked_platform_flags = sloked_posix_platform_flags
subdir('components/compat')

subdir('components/frontend-cpp')
subdir('components/bootstrap')
