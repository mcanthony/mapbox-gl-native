#include "../platform/default/default_styles.hpp"

#include "mapwindow.hpp"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QString>

MapWindow::MapWindow()
{
    connect(&m_map, SIGNAL(needsRendering()), this, SLOT(updateGL()));

    m_map.setAccessToken(qgetenv("MAPBOX_ACCESS_TOKEN"));
    m_map.setCacheDatabase("/tmp/mbgl-cache.db");

    // Set default location to Helsinki.
    m_map.setCoordinateZoom(QPointF(60.170448, 24.942046), 14);

    changeStyle();
}

void MapWindow::changeStyle()
{
    static uint8_t currentStyleIndex;

    const auto& newStyle = mbgl::util::defaultStyles[currentStyleIndex];
    QString url(newStyle.first.c_str());

    m_map.setStyleURL(url);

    QString name(newStyle.second.c_str());
    setWindowTitle(QString("Mapbox GL: ") + name);

    if (++currentStyleIndex == mbgl::util::defaultStyles.size()) {
        currentStyleIndex = 0;
    }
}

void MapWindow::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_S) {
        changeStyle();
    }

    ev->accept();
}

void MapWindow::mousePressEvent(QMouseEvent *ev)
{
#if QT_VERSION < 0x050000
    m_lastPos = ev->posF();
#else
    m_lastPos = ev->localPos();
#endif

    if (ev->type() == QEvent::MouseButtonPress) {
        if (ev->buttons() == (Qt::LeftButton | Qt::RightButton)) {
            changeStyle();
        }
    }

    if (ev->type() == QEvent::MouseButtonDblClick) {
        if (ev->buttons() == Qt::LeftButton) {
            m_map.scaleBy(2.0, m_lastPos, 500);
        } else if (ev->buttons() == Qt::RightButton) {
            m_map.scaleBy(0.5, m_lastPos, 500);
        }
    }

    ev->accept();
}

void MapWindow::mouseMoveEvent(QMouseEvent *ev)
{
#if QT_VERSION < 0x050000
    QPointF delta = ev->posF() - m_lastPos;
#else
    QPointF delta = ev->localPos() - m_lastPos;
#endif

    if (!delta.isNull()) {
        if (ev->buttons() == Qt::LeftButton) {
            m_map.moveBy(delta);
        } else if (ev->buttons() == Qt::RightButton) {
#if QT_VERSION < 0x050000
            m_map.rotateBy(m_lastPos, ev->posF());
#else
            m_map.rotateBy(m_lastPos, ev->localPos());
#endif
        }
    }

#if QT_VERSION < 0x050000
    m_lastPos = ev->posF();
#else
    m_lastPos = ev->localPos();
#endif
    ev->accept();
}

void MapWindow::wheelEvent(QWheelEvent *ev)
{
    if (ev->orientation() == Qt::Horizontal) {
        return;
    }

    float factor = ev->delta() / 1200.;
    if (ev->delta() < 0) {
        factor = factor > -1 ? factor : 1 / factor;
    }

    m_map.scaleBy(1 + factor, ev->pos(), 50);
    ev->accept();
}

void MapWindow::resizeGL(int w, int h)
{
    m_map.resize(QSize(w, h));
}

void MapWindow::paintGL()
{
    m_map.render();
}
