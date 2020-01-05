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
subdir('components/demo')
