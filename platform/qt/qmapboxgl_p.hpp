#ifndef QMAPBOXGL_P_H
#define QMAPBOXGL_P_H

#include "qfilesource_p.hpp"

#include <mbgl/map/map.hpp>
#include <mbgl/map/view.hpp>
#include <mbgl/platform/qt/qmapboxgl.hpp>
#include <mbgl/util/run_loop.hpp>

#include <QSize>

namespace mbgl {

class Map;
class FileSource;

}  // namespace mbgl

class QMapboxGLPrivate : public QObject, public mbgl::View
{
    Q_OBJECT

public:
    explicit QMapboxGLPrivate(QMapboxGL *q);
    virtual ~QMapboxGLPrivate();

    // mbgl::View implementation.
    float getPixelRatio() const final;
    std::array<uint16_t, 2> getSize() const final;
    std::array<uint16_t, 2> getFramebufferSize() const final;

    void activate() final {}
    void deactivate() final {}
    void notify() final {}
    void invalidate() final;
    void beforeRender() final {}
    void afterRender() final {}
    void notifyMapChange(mbgl::MapChange change) final;

    QSize size;

    QMapboxGL *q_ptr = nullptr;

    QFileSourcePrivate fileSourceObj;

    mbgl::util::RunLoop loop;
    mbgl::Map mapObj;

signals:
    void mapRegionDidChange();
    void needsRendering();
};

#endif // QMAPBOXGL_P_H
