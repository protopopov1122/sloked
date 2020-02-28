sloked_tests_src = [
    'main.cpp',
    'core/Base64.cpp',
    'core/Cache.cpp',
    'core/CharPreset.cpp',
    'core/Counter.cpp'
]

sloked_framework_tests = executable('sloked_framework_tests', sloked_tests_src,
  include_directories : include_directories('../../../third-party'),
  dependencies : sloked_framework)

test('Framework unit tests', sloked_framework_tests)