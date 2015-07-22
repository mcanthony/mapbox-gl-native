{
  'includes': [
    '../qt/qmapboxgl.gypi',
    '../linux/mapboxgl-app.gypi',
  ],

  'conditions': [
    ['test', { 'includes': [ '../test/test.gypi' ] } ],
    ['render', { 'includes': [ '../bin/render.gypi' ] } ],
  ],
}
