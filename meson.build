project('tmenu', 'c',
  version: '0.1',
  default_options: ['warning_level=3'])

src = [
  'tmenu.c',
]

deps = [
  dependency('readline'),
  dependency('tinfo'),
]

executable('tmenu', src,
  dependencies: deps,
  install: true)
