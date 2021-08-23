shmoggl_sources = [
  'main.c',
  'shmoggl-window.c',
  'shmoggl-database.c',
  'shmoggl-task.c',
  'shmoggl-task-view.c',
  'shmoggl-time-slice.c',
  'shmoggl-time-slice-view.c',
  'shmoggl-utils.c',
]

shmoggl_deps = [
  dependency('gio-2.0', version: '>= 2.50'),
  dependency('gtk+-3.0', version: '>= 3.22'),
  dependency('threads'),
]

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
shmoggl_deps += m_dep
dl_dep = cc.find_library('dl')
shmoggl_deps += dl_dep

gnome = import('gnome')

shmoggl_sources += gnome.compile_resources('shmoggl-resources',
  'shmoggl.gresource.xml',
  c_name: 'shmoggl'
)

subdir('dependencies')

executable('shmoggl', shmoggl_sources,
  dependencies: shmoggl_deps,
  install: true,
)
