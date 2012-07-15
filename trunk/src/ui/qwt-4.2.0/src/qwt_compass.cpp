/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#include <math.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qevent.h>
#include "qwt_math.h"
#include "qwt_scldraw.h"
#include "qwt_paint_buffer.h"
#include "qwt_painter.h"
#include "qwt_dial_needle.h"
#include "qwt_compass_rose.h"
#include "qwt_compass.h"

/*!
  \brief Constructor
  \param parent Parent widget
  \param name Widget name

  Create a compass widget with a scale, no needle and no rose. 
  The default origin is 270.0 with no valid value. It accepts
  mouse and keyboard inputs and has no step size. The default mode
  is QwtDial::RotateNeedle.
*/  
QwtCompass::QwtCompass(QWidget* parent, const char* name):
        QwtDial(parent, name),
    d_rose(0)
{
    setScaleOptions(ScaleLabel); // Only labels, no backbone, no ticks

    setOrigin(270.0);
    setWrapping(TRUE);


    d_labelMap.insert(0.0, QString::fromLatin1("N"));
    d_labelMap.insert(45.0, QString::fromLatin1("NE"));
    d_labelMap.insert(90.0, QString::fromLatin1("E"));
    d_labelMap.insert(135.0, QString::fromLatin1("SE"));
    d_labelMap.insert(180.0, QString::fromLatin1("S"));
    d_labelMap.insert(225.0, QString::fromLatin1("SW"));
    d_labelMap.insert(270.0, QString::fromLatin1("W"));
    d_labelMap.insert(315.0, QString::fromLatin1("NW"));

#if 0
    d_labelMap.insert(22.5, QString::fromLatin1("NNE"));
    d_labelMap.insert(67.5, QString::fromLatin1("NEE"));
    d_labelMap.insert(112.5, QString::fromLatin1("SEE"));
    d_labelMap.insert(157.5, QString::fromLatin1("SSE"));
    d_labelMap.insert(202.5, QString::fromLatin1("SSW"));
    d_labelMap.insert(247.5, QString::fromLatin1("SWW"));
    d_labelMap.insert(292.5, QString::fromLatin1("NWW"));
    d_labelMap.insert(337.5, QString::fromLatin1("NNW"));
#endif
}

//!  Destructor
QwtCompass::~QwtCompass() 
{
    delete d_rose;
}

//! Draw the contents of the scale
void QwtCompass::drawScaleContents(QPainter *painter, 
        const QPoint &center, int radius) const
{
    QPalette::ColorGroup cg;
    if ( isEnabled() )
        cg = hasFocus() ? QPalette::Active : QPalette::Inactive;
    else
        cg = QPalette::Disabled;

    double north = origin();
    if ( isValid() )
    {
        if ( mode() == RotateScale )
            north -= value(); 
    }

    const int margin = 4;
    drawRose(painter, center, radius - margin, 360.0 - north,  cg);
}

/*!
  Draw the compass rose
  
  \param painter Painter
  \param center Center of the compass
  \param radius of the circle, where to paint the rose
  \param north Direction pointing north, in degrees counter clockwise
  \param cg Color group
*/
void QwtCompass::drawRose(QPainter *painter, const QPoint &center,
    int radius, double north, QPalette::ColorGroup cg) const
{
    if ( d_rose )
        d_rose->draw(painter, center, radius, north,  cg);
}

/*!
  Set a rose for the compass
  \param rose Compass rose
  \warning The rose will be deleted, when a different rose is
    set or in ~QwtCompass
  \sa QwtCompass::rose
*/
void QwtCompass::setRose(QwtCompassRose *rose)
{
    if ( rose != d_rose )
    {
        if ( d_rose )
            delete d_rose;

        d_rose = rose;
        update();
    }
}

/*! 
  \return rose
  \sa QwtCompass::setRose
*/
const QwtCompassRose *QwtCompass::rose() const 
{ 
    return d_rose; 
}

/*! 
  \return rose
  \sa QwtCompass::setRose
*/
QwtCompassRose *QwtCompass::rose() 
{ 
    return d_rose; 
}

/*! 
  Handles key events

  Beside the keys described in QwtDial::keyPressEvent numbers
  from 1-9 (without 5) set the direction according to their
  position on the num pad.

  \sa isReadOnly()
*/
void QwtCompass::keyPressEvent(QKeyEvent *kev) 
{
    if (isReadOnly()) 
        return;

#if 0
    if ( kev->key() == Key_5 )
    {
        invalidate(); // signal ???
        return;
    }
#endif

    double newValue = value();

    if ( kev->key() >= Qt::Key_1 && kev->key() <= Qt::Key_9 )
    {
        if ( mode() != RotateNeedle || kev->key() == Qt::Key_5 )
            return;

        switch (kev->key()) 
        {
            case Qt::Key_6: 
                newValue = 180.0 * 0.0;
                break;
            case Qt::Key_3: 
                newValue = 180.0 * 0.25;
                break;
            case Qt::Key_2: 
                newValue = 180.0 * 0.5;
                break;
            case Qt::Key_1: 
                newValue = 180.0 * 0.75;
                break;
            case Qt::Key_4: 
                newValue = 180.0 * 1.0;
                break;
            case Qt::Key_7: 
                newValue = 180.0 * 1.25;
                break;
            case Qt::Key_8: 
                newValue = 180.0 * 1.5;
                break;
            case Qt::Key_9: 
                newValue = 180.0 * 1.75;
                break;
        }
        newValue -= origin();
        setValue(newValue);
    }
    else
    {
        QwtDial::keyPressEvent(kev);
    }
}

/*!
  \return map, mapping values to labels
  \sa QwtCompass::setLabelMap
*/
const QMap<double, QString> &QwtCompass::labelMap() const 
{ 
    return d_labelMap; 
}

/*!
  \return map, mapping values to labels
  \sa QwtCompass::setLabelMap
*/
QMap<double, QString> &QwtCompass::labelMap() 
{ 
    return d_labelMap; 
}

/*!
  \brief Set a map, mapping values to labels
  \param map value to label map

  The values of the major ticks are found by looking into this
  map. The default map consists of the labels N, NE, E, SE, S, SW, W, NW.

  \warning The map will have no effect for values that are no major
           tick values. Major ticks can be changed by QwtScaleDraw::setScale

  \sa QwtCompass::labelMap, QwtCompass::scaleDraw, QwtScaleDraw::setScale
*/
void QwtCompass::setLabelMap(const QMap<double, QString> &map) 
{ 
    d_labelMap = map; 
}

/*!
  Map a value to a corresponding label
  \param value Value that will be mapped
  \return Label, or QString::null

  label() looks in a map for a corresponding label for value
  or return QString::null.
  \sa QwtCompass::labelMap, QwtCompass::setLabelMap
*/

QString QwtCompass::scaleLabel(double value) const
{
#if 0
    // better solution ???
    if ( value == -0 )
        value = 0.0;
#endif

    if ( value < 0.0 )
        value += 360.0;

    if ( d_labelMap.contains(value) )
        return d_labelMap[value];

    return QString::null;
}

// Local Variables:
// mode: C++
// c-file-style: "stroustrup"
// indent-tabs-mode: nil
// End:
