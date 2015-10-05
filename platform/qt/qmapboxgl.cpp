#include "qmapboxgl_p.hpp"

#include <mbgl/annotation/point_annotation.hpp>
#include <mbgl/annotation/sprite_image.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/platform/gl.hpp>
#include <mbgl/platform/qt/qmapboxgl.hpp>
#include <mbgl/util/geo.hpp>
#include <mbgl/util/vec.hpp>

#include <QImage>
#include <QString>

#include <chrono>

QMapboxGL::QMapboxGL(QObject *parent_) : QObject(parent_), d_ptr(new QMapboxGLPrivate(this))
{
}

QMapboxGL::~QMapboxGL()
{
}

void QMapboxGL::setAccessToken(const QString &token)
{
    d_ptr->fileSourceObj.setAccessToken(token);
}

void QMapboxGL::setCacheDatabase(const QString &path)
{
    d_ptr->fileSourceObj.setCacheDatabase(path);
}

void QMapboxGL::setStyleJSON(const QString &style)
{
    d_ptr->mapObj.setStyleJSON(style.toUtf8().constData());
}

void QMapboxGL::setStyleURL(const QString &url)
{
    d_ptr->mapObj.setStyleURL(url.toUtf8().constData());
}

double QMapboxGL::latitude() const
{
    return d_ptr->mapObj.getLatLng().latitude;
}

void QMapboxGL::setLatitude(double latitude_)
{
    d_ptr->mapObj.setLatLng({ latitude_, longitude() });
}

double QMapboxGL::longitude() const
{
    return d_ptr->mapObj.getLatLng().longitude;
}

void QMapboxGL::setLongitude(double longitude_)
{
    d_ptr->mapObj.setLatLng({ latitude(), longitude_ });
}

double QMapboxGL::scale() const
{
    return d_ptr->mapObj.getScale();
}

void QMapboxGL::setScale(double scale_, const QPointF &centerPixel, int milliseconds)
{
    d_ptr->mapObj.setScale(scale_, centerPixel.x(), centerPixel.y(), std::chrono::milliseconds(milliseconds));
}

double QMapboxGL::zoom() const
{
    return d_ptr->mapObj.getZoom();
}

void QMapboxGL::setZoom(double zoom_, int milliseconds)
{
    d_ptr->mapObj.setZoom(zoom_, std::chrono::milliseconds(milliseconds));
}

double QMapboxGL::minimumZoom() const
{
    return d_ptr->mapObj.getMinZoom();
}

double QMapboxGL::maximumZoom() const
{
    return d_ptr->mapObj.getMaxZoom();
}

QMapboxGL::Coordinate QMapboxGL::coordinate() const
{
    const mbgl::LatLng& latLng = d_ptr->mapObj.getLatLng();

    return Coordinate(latLng.latitude, latLng.longitude);
}

void QMapboxGL::setCoordinate(const QPointF &coordinate_, int milliseconds)
{
    d_ptr->mapObj.setLatLng(
        { coordinate_.x(), coordinate_.y() }, std::chrono::milliseconds(milliseconds));
}

void QMapboxGL::setCoordinateZoom(const QPointF &coordinate_, double zoom_, int milliseconds)
{
    d_ptr->mapObj.setLatLngZoom(
        { coordinate_.x(), coordinate_.y() }, zoom_, std::chrono::milliseconds(milliseconds));
}

double QMapboxGL::bearing() const
{
    return d_ptr->mapObj.getBearing();
}

void QMapboxGL::setBearing(double degrees, int milliseconds)
{
    d_ptr->mapObj.setBearing(degrees, std::chrono::milliseconds(milliseconds));
}

void QMapboxGL::setBearing(double degrees, const QPointF &centerPixel)
{
    d_ptr->mapObj.setBearing(degrees, centerPixel.x(), centerPixel.y());
}

double QMapboxGL::pitch() const
{
    return d_ptr->mapObj.getPitch();
}

void QMapboxGL::setPitch(double pitch_, int milliseconds)
{
    d_ptr->mapObj.setPitch(pitch_, std::chrono::milliseconds(milliseconds));
}

void QMapboxGL::setGestureInProgress(bool inProgress)
{
    d_ptr->mapObj.setGestureInProgress(inProgress);
}

bool QMapboxGL::isRotating() const
{
    return d_ptr->mapObj.isRotating();
}

bool QMapboxGL::isScaling() const
{
    return d_ptr->mapObj.isScaling();
}

bool QMapboxGL::isPanning() const
{
    return d_ptr->mapObj.isPanning();
}

bool QMapboxGL::isFullyLoaded() const
{
    return d_ptr->mapObj.isFullyLoaded();
}

void QMapboxGL::moveBy(const QPointF &offset)
{
    d_ptr->mapObj.moveBy(offset.x(), offset.y());
}

void QMapboxGL::scaleBy(double scale_, const QPointF &centerPixel, int milliseconds) {
    d_ptr->mapObj.scaleBy(
        scale_, centerPixel.x(), centerPixel.y(), std::chrono::milliseconds(milliseconds));
}

void QMapboxGL::rotateBy(const QPointF &lastPosition, const QPointF &currentPosition)
{
    d_ptr->mapObj.rotateBy(
        lastPosition.x(), lastPosition.y(), currentPosition.x(), currentPosition.y());
}

void QMapboxGL::resize(const QSize& size)
{
    if (d_ptr->size == size) {
        return;
    }

    d_ptr->size = size;
    d_ptr->mapObj.update(mbgl::Update::Dimensions);
}

void QMapboxGL::setSprite(const QString &name, const QImage &sprite)
{
    if (sprite.isNull()) {
        return;
    }

    const QImage swapped = sprite.rgbSwapped();

    d_ptr->mapObj.setSprite(name.toUtf8().constData(), std::make_shared<mbgl::SpriteImage>(
        swapped.width(), swapped.height(), 1.0,
        std::string(reinterpret_cast<const char*>(swapped.constBits()), swapped.byteCount())));
}

quint32 QMapboxGL::addPointAnnotation(const QString &name, const QPointF &position) {
    return d_ptr->mapObj.addPointAnnotation({ { position.x(), position.y() }, name.toUtf8().data() });
}

QPointF QMapboxGL::pixelForCoordinate(const Coordinate &coordinate_) const
{
    const mbgl::vec2<double> pixel =
        d_ptr->mapObj.pixelForLatLng({ coordinate_.first, coordinate_.second });

    return QPointF(pixel.x, d_ptr->size.height() - pixel.y);
}

QMapboxGL::Coordinate QMapboxGL::coordinateForPixel(const QPointF &pixel) const
{
    const mbgl::LatLng latLng =
        d_ptr->mapObj.latLngForPixel({ pixel.x(), d_ptr->size.height() - pixel.y() });

    return Coordinate(latLng.latitude, latLng.longitude);
}

void QMapboxGL::render()
{
    d_ptr->mapObj.renderSync();
}

QMapboxGLPrivate::QMapboxGLPrivate(QMapboxGL *q)
    : QObject(q)
    , q_ptr(q)
    , mapObj(*this, fileSourceObj)
{
    connect(this, SIGNAL(needsRendering()), q_ptr, SIGNAL(needsRendering()), Qt::DirectConnection);
    connect(this, SIGNAL(mapRegionDidChange()), q_ptr, SIGNAL(mapRegionDidChange()), Qt::DirectConnection);
}

QMapboxGLPrivate::~QMapboxGLPrivate()
{
}

float QMapboxGLPrivate::getPixelRatio() const
{
    // FIXME: Should handle pixel ratio.
    return 1.0;
}

std::array<uint16_t, 2> QMapboxGLPrivate::getSize() const
{
    return {{ static_cast<uint16_t>(size.width()), static_cast<uint16_t>(size.height()) }};
}

std::array<uint16_t, 2> QMapboxGLPrivate::getFramebufferSize() const
{
    return getSize();
}

void QMapboxGLPrivate::invalidate()
{
    emit needsRendering();
}

void QMapboxGLPrivate::notifyMapChange(mbgl::MapChange change)
{
    // Map thread.
    switch (change) {
    case mbgl::MapChangeRegionDidChange:
    case mbgl::MapChangeRegionDidChangeAnimated:
    case mbgl::MapChangeRegionIsChanging:
        emit mapRegionDidChange();
        break;
    default:
        break;
    }
}
