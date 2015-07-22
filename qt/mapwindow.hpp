#ifndef MAPWINDOW_H
#define MAPWINDOW_H

#include "../platform/default/default_styles.hpp"

#include <mbgl/platform/qt/QMapboxGL>

#include <QGLWidget>

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

class MapWindow : public QGLWidget, public QMapboxGL::Observer
{
public:
    MapWindow();

private:
    void changeStyle();

    // QGLWidget implementation.
    void keyPressEvent(QKeyEvent *ev) final;
    void mousePressEvent(QMouseEvent *ev) final;
    void mouseMoveEvent(QMouseEvent *ev) final;
    void wheelEvent(QWheelEvent *ev) final;
    void resizeGL(int w, int h) final;
    void paintGL() final;

    // QMapboxGL::Observer implementation.
    void activated() final {}
    void deactivated() final {}
    void beforeRendering() final;
    void afterRendering() final;

    QPointF m_lastPos;

    QMapboxGL m_map;
};

#endif
