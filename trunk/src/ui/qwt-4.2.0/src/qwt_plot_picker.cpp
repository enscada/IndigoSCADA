/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include "qwt_plot.h"
#include "qwt_double_rect.h"
#include "qwt_painter.h"
#include "qwt_array.h"
#include "qwt_plot_picker.h"

/*!
  \brief Create a plot picker

  The picker is set to those x- and y-axis of the plot
  that are enabled. If both or no x-axis are enabled, the picker
  is set to QwtPlot::xBottom. If both or no y-axis are
  enabled, it is set to QwtPlot::yLeft.

  \param canvas Plot canvas to observe, also the parent object
  \param name Object name

  \warning Calling QwtPlot::setAxisScale() while QwtPlot::autoReplot() is FALSE
           leaves the axis in an 'intermediate' state.
           In this case, to prevent buggy behaviour, you must call
           QwtPlot::replot() before calling QwtPlotPicker().
           This quirk will be removed in a future release.

  \sa QwtPlot::autoReplot(), QwtPlot::replot(), QwtPlotPicker::scaleRect()
*/
  
QwtPlotPicker::QwtPlotPicker(QwtPlotCanvas *canvas, const char *name):
    QwtPicker(canvas, name),
    d_xAxis(-1),
    d_yAxis(-1)
{
    if ( !canvas )
        return;

    // attach axes

    int xAxis = QwtPlot::xBottom;

    const QwtPlot *plot = QwtPlotPicker::plot();
    if ( !plot->axisEnabled(QwtPlot::xBottom) &&
        plot->axisEnabled(QwtPlot::xTop) )
    {
        xAxis = QwtPlot::xTop;
    }

    int yAxis = QwtPlot::yLeft;
    if ( !plot->axisEnabled(QwtPlot::yLeft) &&
        plot->axisEnabled(QwtPlot::yRight) )
    {
        yAxis = QwtPlot::yRight;
    }

    setAxis(xAxis, yAxis);
}

/*!
  Create a plot picker

  \param xAxis Set the x axis of the picker
  \param yAxis Set the y axis of the picker
  \param canvas Plot canvas to observe, also the parent object
  \param name Object name

  \warning Calling QwtPlot::setAxisScale() while QwtPlot::autoReplot() is FALSE
           leaves the axis in an 'intermediate' state.
           In this case, to prevent buggy behaviour, you must call
           QwtPlot::replot() before calling QwtPlotPicker().
           This quirk will be removed in a future release.

  \sa QwtPlot::autoReplot(), QwtPlot::replot(), QwtPlotPicker::scaleRect()
*/
QwtPlotPicker::QwtPlotPicker(int xAxis, int yAxis, 
        QwtPlotCanvas *canvas, const char *name):
    QwtPicker(canvas, name),
    d_xAxis(xAxis),
    d_yAxis(yAxis)
{
}

/*!
  Create a plot picker

  \param xAxis X axis of the picker
  \param yAxis Y axis of the picker
  \param selectionFlags Or�d value of SelectionType, RectSelectionType and
                        SelectionMode
  \param rubberBand Rubberband style
  \param cursorLabelMode Cursor label mode
  \param canvas Plot canvas to observe, also the parent object
  \param name Object name

  \sa QwtPicker, QwtPicker::setSelectionFlags(), QwtPicker::setRubberBand(),
      QwtPicker::setCursorLabelMode

  \warning Calling QwtPlot::setAxisScale() while QwtPlot::autoReplot() is FALSE
           leaves the axis in an 'intermediate' state.
           In this case, to prevent buggy behaviour, you must call
           QwtPlot::replot() before calling QwtPlotPicker().
           This quirk will be removed in a future release.

  \sa QwtPlot::autoReplot(), QwtPlot::replot(), QwtPlotPicker::scaleRect()
*/
QwtPlotPicker::QwtPlotPicker(int xAxis, int yAxis, int selectionFlags,
        RubberBand rubberBand, DisplayMode cursorLabelMode,
        QwtPlotCanvas *canvas, const char *name):
    QwtPicker(selectionFlags, rubberBand, cursorLabelMode, canvas, name),
    d_xAxis(xAxis),
    d_yAxis(yAxis)
{
}

//! Return observed plot canvas
QwtPlotCanvas *QwtPlotPicker::canvas()
{
    QWidget *w = parentWidget();
    if ( w && w->inherits("QwtPlotCanvas") )
        return (QwtPlotCanvas *)w;

    return NULL;
}

//! Return Observed plot canvas
const QwtPlotCanvas *QwtPlotPicker::canvas() const
{
    return ((QwtPlotPicker *)this)->canvas();
}

//! Return plot widget, containing the observed plot canvas
QwtPlot *QwtPlotPicker::plot()
{
    QObject *w = canvas();
    if ( w )
    {
        w = w->parent();
        if ( w && w->inherits("QwtPlot") )
            return (QwtPlot *)w;
    }

    return NULL;
}

//! Return plot widget, containing the observed plot canvas
const QwtPlot *QwtPlotPicker::plot() const
{
    return ((QwtPlotPicker *)this)->plot();
}

/*!
  Return normalized bounding rect of the axes

  \warning Calling QwtPlot::setAxisScale() while QwtPlot::autoReplot() is FALSE
           leaves the axis in an 'intermediate' state.
           In this case, to prevent buggy behaviour, you must call
           QwtPlot::replot() before calling QwtPlotPicker::scaleRect().
           This quirk will be removed in a future release.

  \sa QwtPlot::autoReplot(), QwtPlot::replot().
*/
QwtDoubleRect QwtPlotPicker::scaleRect() const
{
    const QwtPlot *plt = plot();

    const QwtDoubleRect rect(
        plt->axisScale(xAxis())->lBound(),
        plt->axisScale(xAxis())->hBound(),
        plt->axisScale(yAxis())->lBound(),
        plt->axisScale(yAxis())->hBound()
    );

    return rect.normalize();
}

/*!
  Set the x and y axes of the picker

  \param xAxis X axis
  \param yAxis Y axis
*/
void QwtPlotPicker::setAxis(int xAxis, int yAxis)
{
    const QwtPlot *plt = plot();
    if ( !plt )
        return;

    if ( xAxis != d_xAxis || yAxis != d_yAxis )
    {
        d_xAxis = xAxis;
        d_yAxis = yAxis;
    }
}

//! Return x axis
int QwtPlotPicker::xAxis() const
{
    return d_xAxis;
}

//! Return y axis
int QwtPlotPicker::yAxis() const
{
    return d_yAxis;
}

/*!
  Translate a pixel position into a position string

  \param pos Position in pixel coordinates
  \return Position string
*/
QString QwtPlotPicker::cursorLabel(const QPoint &pos) const
{
    return cursorLabel(invTransform(pos));
}

/*!
  \brief Translate a position into a position string

  In case of HLineRubberBand the label is the value of the
  y position, in case of VLineRubberBand the value of the x position.
  Otherwise the label contains x and y position separated by a �, �.

  The format for the double to string conversion is "%.4f".

  \param pos Position
  \return Position string
*/
QString QwtPlotPicker::cursorLabel(const QwtDoublePoint &pos) const
{
    switch(rubberBand())
    {
        case HLineRubberBand:
            return QString().sprintf("%.4f", pos.y());
        case VLineRubberBand:
            return QString().sprintf("%.4f", pos.x());
        default:
            return QString().sprintf("%.4f, %.4f", pos.x(), pos.y());
    }
    return QString::null; // make some dumb compilers happy
}

/*! 
  Append a point to the selection and update rubberband and cursor label.
    
  \param pos Additional point
  \sa isActive, begin(), end(), move(), appended()

  \note The appended(const QPoint &), appended(const QDoublePoint &) 
        signals are emitted.
*/
void QwtPlotPicker::append(const QPoint &pos)
{
    QwtPicker::append(pos);
    emit appended(invTransform(pos));
}

/*!
  Move the last point of the selection

  \param pos New position
  \sa isActive, begin(), end(), append()

  \note The moved(const QPoint &), moved(const QDoublePoint &) 
        signals are emitted.
*/
void QwtPlotPicker::move(const QPoint &pos)
{
    QwtPicker::move(pos);
    emit moved(invTransform(pos));
}

/*!
  Close a selection setting the state to inactive.

  \param ok If TRUE, complete the selection and emit selected signals
            otherwise discard the selection.
  \return TRUE if the selection is accepted, FALSE otherwise
*/

bool QwtPlotPicker::end(bool ok)
{
    ok = QwtPicker::end(ok);
    if ( !ok )
        return FALSE;

    QwtPlot *plot = QwtPlotPicker::plot();
    if ( !plot )
        return FALSE;

    const QPointArray &pa = selection();
    if ( pa.count() == 0 )
        return FALSE;

    if ( selectionFlags() & PointSelection )
    {
        const QwtDoublePoint pos = invTransform(pa[0]);
        emit selected(pos);
    }
    else if ( (selectionFlags() & RectSelection) && pa.count() >= 2 )
    {
        QPoint p1 = pa[0];
        QPoint p2 = pa[int(pa.count() - 1)];

        if ( selectionFlags() & CenterToCorner )
        {
            p1.setX(p1.x() - (p2.x() - p1.x()));
            p1.setY(p1.y() - (p2.y() - p1.y()));
        }
        else if ( selectionFlags() & CenterToRadius )
        {
            const int radius = QMAX(QABS(p2.x() - p1.x()),
                QABS(p2.y() - p1.y()));
            p2.setX(p1.x() + radius);
            p2.setY(p1.y() + radius);
            p1.setX(p1.x() - radius);
            p1.setY(p1.y() - radius);
        }

        emit selected(invTransform(QRect(p1, p2)).normalize());
    }
    else 
    {
        QwtArray<QwtDoublePoint> dpa(pa.count());
        for ( int i = 0; i < int(pa.count()); i++ )
            dpa[i] = invTransform(pa[i]);

        emit selected(dpa);
    }

    return TRUE;
}

/*!
    Translate a rectangle from pixel into plot coordinates

    \return Rectangle in plot coordinates
    \sa QwtPlotPicker::transform()
*/
QwtDoubleRect QwtPlotPicker::invTransform(const QRect &rect) const
{
    QwtDiMap xMap = plot()->canvasMap(d_xAxis);
    QwtDiMap yMap = plot()->canvasMap(d_yAxis);

    return QwtDoubleRect(
        xMap.invTransform(rect.left()),
        xMap.invTransform(rect.right()),
        yMap.invTransform(rect.top()),
        yMap.invTransform(rect.bottom())
    );
}

/*!
    Translate a rectangle from plot into pixel coordinates
    \return Rectangle in pixel coordinates
    \sa QwtPlotPicker::invTransform()
*/
QRect QwtPlotPicker::transform(const QwtDoubleRect &rect) const
{
    QwtDiMap xMap = plot()->canvasMap(d_xAxis);
    QwtDiMap yMap = plot()->canvasMap(d_yAxis);

    const int x1 = xMap.transform(rect.x1());
    const int x2 = xMap.transform(rect.x2());
    const int y1 = yMap.transform(rect.y1());
    const int y2 = yMap.transform(rect.y2());

    return QRect(x1, y1, x2 - x1, y2 - y1);
}

/*!
    Translate a point from pixel into plot coordinates
    \return Point in plot coordinates
    \sa QwtPlotPicker::transform()
*/
QwtDoublePoint QwtPlotPicker::invTransform(const QPoint &pos) const
{
    QwtDiMap xMap = plot()->canvasMap(d_xAxis);
    QwtDiMap yMap = plot()->canvasMap(d_yAxis);

    return QwtDoublePoint(
        xMap.invTransform(pos.x()),
        yMap.invTransform(pos.y())
    );
}

/*!
    Translate a point from plot into pixel coordinates
    \return Point in pixel coordinates
    \sa QwtPlotPicker::invTransform()
*/
QPoint QwtPlotPicker::transform(const QwtDoublePoint &pos) const
{
    QwtDiMap xMap = plot()->canvasMap(d_xAxis);
    QwtDiMap yMap = plot()->canvasMap(d_yAxis);

    return QPoint(
        xMap.transform(pos.x()),
        yMap.transform(pos.y())
    );
}

// Local Variables:
// mode: C++
// c-file-style: "stroustrup"
// indent-tabs-mode: nil
// End:
