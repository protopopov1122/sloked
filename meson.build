project('sloked', 'cpp', 'c', default_options : ['cpp_std=c++17', 'warning_level=3'])

compiler_id = meson.get_compiler('cpp').get_id()
if compiler_id == 'gcc' or compiler_id == 'clang'
  add_project_arguments('-Wno-unused-parameter', '-pedantic', language : 'cpp')
  add_project_link_arguments('-rdynamic', language : 'cpp')
elif compiler_id == 'pgi'
  add_project_arguments('-std=c++17', '-noswitcherror', language : 'cpp')
  add_project_link_arguments('-noswitcherror', '-rdynamic', language : 'cpp')
endif

subdir('components/framework')
subdir('components/crypto/botan')
subdir('components/platform/posix')
subdir('components/script/lua')

sloked_crypto = sloked_botan_crypto
sloked_crypto_flags = sloked_botan_crypto_flags
sloked_platform = sloked_posix_platform
sloked_platform_flags = sloked_posix_platform_flags
sloked_scripting = sloked_lua_bindings
sloked_scripting_flags = sloked_lua_bindings_flags
subdir('components/compat')

subdir('components/demo')
