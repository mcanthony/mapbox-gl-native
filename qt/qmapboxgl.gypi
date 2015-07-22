{
  'includes': [
    '../gyp/common.gypi',
  ],
  'targets': [
    { 
      'target_name': 'qtapp',
      'product_name': 'qmapboxgl',
      'type': 'executable',

      'includes': [
        '../gyp/qt.gypi',
      ],

      'dependencies': [
        '../mbgl.gyp:core',
        '../mbgl.gyp:platform-<(platform_lib)',
        '../mbgl.gyp:http-<(http_lib)',
        '../mbgl.gyp:asset-<(asset_lib)',
        '../mbgl.gyp:cache-<(cache_lib)',
        '../mbgl.gyp:copy_styles',
        '../mbgl.gyp:copy_certificate_bundle',
      ],

      'sources': [
        'main.cpp',
        'mapwindow.cpp',
        'mapwindow.hpp',
        '../include/mbgl/platform/qt/qmapboxgl.hpp',
        '../platform/default/default_styles.cpp',
        '../platform/default/log_stderr.cpp',
        '../platform/qt/qmapboxgl.cpp',
        '../platform/qt/qmapboxgl_p.hpp',
      ],

      'include_dirs': [
        '../include',
        '../src',
      ],

      'conditions': [
        ['OS == "linux"', {
          'cflags_cc': [
            '<@(opengl_cflags)',
            '<@(qt_cflags)',
            '-Wno-error'
          ],

          'libraries': [
            '<@(opengl_ldflags)',
            '<@(qt_ldflags)'
          ],
        }],
        ['OS == "mac"', {
          'xcode_settings': {
            'OTHER_CPLUSPLUSFLAGS': [
              '<@(qt_cflags)',
              '-Wno-error'
            ],
            'OTHER_LDFLAGS': [
              '<@(qt_ldflags)'
            ],
          }
        }],
      ],
    },
  ],
}
