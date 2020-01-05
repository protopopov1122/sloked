project('sloked', 'cpp', 'c', default_options : ['cpp_std=c++17', 'warning_level=3'])

compiler_id = meson.get_compiler('cpp').get_id()
if compiler_id == 'gcc' or compiler_id == 'clang'
  add_project_arguments('-Wno-unused-parameter', '-pedantic', language : 'cpp')
  add_project_link_arguments('-rdynamic', language : 'cpp')
elif compiler_id == 'pgi'
  add_project_arguments('-std=c++17', '-noswitcherror', language : 'cpp')
  add_project_link_arguments('-noswitcherror', '-rdynamic', language : 'cpp')
endif

subdir('src/framework')
subdir('src/crypto/botan')
subdir('src/platform/posix')
subdir('src/script/lua')
subdir('src/demo')
