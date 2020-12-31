project('tmenu', 'c',
  version: '0.1',
  default_options: ['warning_level=3'])

src = [
  'input-readline.c',
  'options.c',
  'redraw.c',
  'terminal-nix.c',
  'tmenu.c',
]

deps = [
  dependency('readline'),
  dependency('tinfo'),
]

executable('tmenu', src,
  dependencies: deps,
  install: true)
