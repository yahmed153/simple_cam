libcamera_public_dep = dependency('libcamera', fallback : ['libcamera', 'libcamera_public'])

project_target = executable(
  meson.project_name(),
  'simple_cam.cpp',
  cpp_args: ['-D_FILE_OFFSET_BITS=64'],
  dependencies: [libcamera_public_dep],
  install : false,
)
