#include "../platform/default/default_styles.hpp"

#include "mapwindow.hpp"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QString>

MapWindow::MapWindow()
{
    setAutoBufferSwap(false);

    m_map.setObserver(this);
    connect(&m_map, SIGNAL(needsRendering()), this, SLOT(updateGL()));

    m_map.setAccessToken(qgetenv("MAPBOX_ACCESS_TOKEN"));

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
    m_lastPos = ev->posF();

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
    QPointF delta = ev->posF() - m_lastPos;

    if (!delta.isNull()) {
        if (ev->buttons() == Qt::LeftButton) {
            m_map.moveBy(delta);
        } else if (ev->buttons() == Qt::RightButton) {
            m_map.rotateBy(m_lastPos, ev->posF());
        }
    }

    m_lastPos = ev->posF();
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

void MapWindow::beforeRendering()
{
    // Mapbox Map thread.
    makeCurrent();
}

void MapWindow::afterRendering()
{
    // Mapbox Map thread.
    swapBuffers();
    doneCurrent();
}
