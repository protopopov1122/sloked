sloked_tests_src = [
    'main.cpp',
    'core/AVL.cpp',
    'core/Base64.cpp',
    'core/CharPreset.cpp',
    'core/Counter.cpp',
    'core/DataHandle.cpp',
    'core/DynamicBitset.cpp',
    'core/Error.cpp',
    'core/Event.cpp',
    'core/OrderedCache.cpp',
    'core/Hash.cpp',
    'core/LRU.cpp',
    'core/Position.cpp',
    'core/RangeMap.cpp',
    'core/RingBuffer.cpp',
    'core/Scope.cpp'
]

threads = dependency('threads')
sloked_framework_tests = executable('sloked_framework_tests', sloked_tests_src,
  include_directories : include_directories('../../../../third-party'),
  dependencies : [threads, sloked_framework])

test('Framework unit tests', sloked_framework_tests)