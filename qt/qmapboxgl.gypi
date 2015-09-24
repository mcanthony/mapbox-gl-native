{
  'variables': {
    'qtlibversion': '<!(git describe --tags --abbrev=0 |cut -dv -f2 |cut -d- -f1)',
    'qtpackagedir': '<(PRODUCT_DIR)/qmapboxgl-<(qtlibversion)',
  },
  'includes': [
    '../gyp/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'qtlib',
      'product_name': 'qmapboxgl',
      'type': 'shared_library',
      'product_extension': 'so.<(qtlibversion)',

      'includes': [
        '../gyp/qt.gypi',
      ],

      'dependencies': [
        '../mbgl.gyp:core',
        '../mbgl.gyp:platform-<(platform_lib)',
        '../mbgl.gyp:asset-<(asset_lib)',
        '../mbgl.gyp:cache-<(cache_lib)',
      ],

      'link_settings': {
        'conditions': [
          ['OS == "mac"', {
            'libraries': [ '-framework OpenGL' ],
            'xcode_settings': { 'OTHER_LDFLAGS': [ '-all_load' ] }
          }]
        ],
      },

    },
    {
      'target_name': 'qtapp',
      'product_name': 'qmapboxgl',
      'type': 'executable',

      'dependencies': [
        '../mbgl.gyp:core',
        '../mbgl.gyp:platform-<(platform_lib)',
        '../mbgl.gyp:asset-<(asset_lib)',
        '../mbgl.gyp:cache-<(cache_lib)',
        '../mbgl.gyp:copy_styles',
        '../mbgl.gyp:copy_certificate_bundle',
      ],

      'sources': [
        'main.cpp',
        'mapwindow.cpp',
        'mapwindow.hpp',
      ],

      'include_dirs': [
        '../include',
      ],

      'variables': {
        'cflags_cc': [
          '<@(opengl_cflags)',
          '<@(qt_cflags)',
          '-Wno-error',
          '-fPIC',
        ],
        'ldflags': [
          '<@(opengl_ldflags)',
          '<@(qt_ldflags)'
        ],
        'libraries': [
        ],
      },

      'conditions': [
        ['OS == "mac"', {
          'libraries': [ '<@(libraries)' ],
          'xcode_settings': {
            'OTHER_CPLUSPLUSFLAGS': [ '<@(cflags_cc)' ],
            'OTHER_LDFLAGS': [ '<@(ldflags)', '-framework OpenGL' ],
          },
        }, {
          'cflags_cc': [ '<@(cflags_cc)' ],
          'libraries': [ '<@(libraries)', '<@(ldflags)' ],
        }],
      ],
    },
    {
      'target_name': 'qtpackage_copy',
      'type': 'none',
      'dependencies': [ 'qtlib' ],
      'copies': [
        {
          'files': [
            '../include/mbgl/platform/qt/QMapboxGL',
            '../include/mbgl/platform/qt/qmapboxgl.hpp',
          ],
          'destination': '<(qtpackagedir)/include/mbgl',
        },
        {
          'files': [
            '<(PRODUCT_DIR)/lib.target/libqmapboxgl.so.<(qtlibversion)',
          ],
          'destination': '<(qtpackagedir)/lib',
        },
      ],
    },
    {
      'target_name': 'qtpackage',
      'type': 'none',
      'dependencies': [ 'qtpackage_copy' ],
      'actions': [
        {
          'action_name': 'Making symlink',
          'inputs': [ '<(qtpackagedir)/lib/libqmapboxgl.so.<(qtlibversion)' ],
          'outputs': [ '<(qtpackagedir)/lib/libqmapboxgl.so' ],
          'action': ['ln', '-f', '-s', 'libqmapboxgl.so.<(qtlibversion)', '<(qtpackagedir)/lib/libqmapboxgl.so' ],
        },
        {
          'action_name': 'Making tarball',
          'inputs': [
            '<(qtpackagedir)/include',
            '<(qtpackagedir)/lib',
          ],
          'outputs': ['<(qtpackagedir).tar.gz'],
          'action': ['tar', '-C', '<(PRODUCT_DIR)', '-czvf', '<(PRODUCT_DIR)/qmapboxgl-<(qtlibversion).tar.gz', 'qmapboxgl-<(qtlibversion)'],
        }
      ],
    },
  ],
}
