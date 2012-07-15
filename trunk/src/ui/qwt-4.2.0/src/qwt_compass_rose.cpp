/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#include <math.h>
#include <qpainter.h>
#include "qwt_math.h"
#include "qwt_painter.h"
#include "qwt_compass_rose.h"

static QPoint cutPoint(QPoint p11, QPoint p12, QPoint p21, QPoint p22)
{
    double dx1 = p12.x() - p11.x();
    double dy1 = p12.y() - p11.y();
    double dx2 = p22.x() - p21.x();
    double dy2 = p22.y() - p21.y();

    if ( dx1 == 0.0 && dx2 == 0.0 )
        return QPoint();

    if ( dx1 == 0.0 )
    {
        const double m = dy2 / dx2;
        const double t = p21.y() - m * p21.x();
        return QPoint(p11.x(), qwtInt(m * p11.x() + t));
    }

    if ( dx2 == 0 )
    {
        const double m = dy1 / dx1;
        const double t = p11.y() - m * p11.x();
        return QPoint(p21.x(), qwtInt(m * p21.x() + t));
    }

    const double m1 = dy1 / dx1;
    const double t1 = p11.y() - m1 * p11.x();

    const double m2 = dy2 / dx2;
    const double t2 = p21.y() - m2 * p21.x();

    if ( m1 == m2 )
        return QPoint();

    const double x = ( t2 - t1 ) / ( m1 - m2 );
    const double y = t1 + m1 * x;

    return QPoint(qwtInt(x), qwtInt(y));
}

QwtSimpleCompassRose::QwtSimpleCompassRose(int numThorns, int numThornLevels):
    d_width(0.2),
    d_numThorns(numThorns),
    d_numThornLevels(numThornLevels),
    d_shrinkFactor(0.9)
{
    const QColor dark(128,128,255);
    const QColor light(192,255,255);
    
    QPalette palette;
    for ( int i = 0; i < QPalette::NColorGroups; i++ )
    {
        palette.setColor((QPalette::ColorGroup)i,
            QColorGroup::Dark, dark);
        palette.setColor((QPalette::ColorGroup)i,
            QColorGroup::Light, light);
    }

    setPalette(palette);
}

void QwtSimpleCompassRose::draw(QPainter *painter, const QPoint &center, 
    int radius, double north, QPalette::ColorGroup cg) const
{
    QColorGroup colorGroup;
    switch(cg)
    {
        case QPalette::Disabled:
            colorGroup = palette().disabled();
        case QPalette::Inactive:
            colorGroup = palette().inactive();
        default:
            colorGroup = palette().active();
    }

    drawRose(painter, colorGroup, center, radius, north, d_width, 
        d_numThorns, d_numThornLevels, d_shrinkFactor);
}

void QwtSimpleCompassRose::drawRose(
    QPainter *painter, const QColorGroup &cg,
    const QPoint &center, int radius, double north, double width,
    int numThorns, int numThornLevels, double shrinkFactor)
{
    if ( numThorns < 4 )
        numThorns = 4;

    if ( numThorns % 4 )
        numThorns += 4 - numThorns % 4;

    if ( numThornLevels <= 0 )
        numThornLevels = numThorns / 4;

    if ( shrinkFactor >= 1.0 )
        shrinkFactor = 1.0;

    if ( shrinkFactor <= 0.5 )
        shrinkFactor = 0.5;

    painter->save();

    painter->setPen(Qt::NoPen);

    for ( int j = 1; j <= numThornLevels; j++ )
    {
        double step =  pow(2.0, j) * M_PI / (double)numThorns;
        if ( step > M_PI_2 )
            break;

        double r = radius;
        for ( int k = 0; k < 3; k++ )
        {
            if ( j + k < numThornLevels )
                r *= shrinkFactor;
        }

        double leafWidth = r * width;
        if ( 2.0 * M_PI / step > 32 )
            leafWidth = 16;

        const double origin = north / 180.0 * M_PI;
        for ( double angle = origin; 
            angle < 2.0 * M_PI + origin; angle += step)
        {
            const QPoint p = qwtPolar2Pos(center, r, angle);
            QPoint p1 = qwtPolar2Pos(center, leafWidth, angle + M_PI_2);
            QPoint p2 = qwtPolar2Pos(center, leafWidth, angle - M_PI_2);

            QPointArray pa(3);
            pa.setPoint(0, center);
            pa.setPoint(1, p);

            QPoint p3 = qwtPolar2Pos(center, r, angle + step / 2.0);
            p1 = cutPoint(center, p3, p1, p);
            pa.setPoint(2, p1);
            painter->setBrush(cg.brush(QColorGroup::Dark));
            painter->drawPolygon(pa);

            QPoint p4 = qwtPolar2Pos(center, r, angle - step / 2.0);
            p2 = cutPoint(center, p4, p2, p);

            pa.setPoint(2, p2);
            painter->setBrush(cg.brush(QColorGroup::Light));
            painter->drawPolygon(pa);
        }
    }
    painter->restore();
}

/**
* Set the width of the rose heads. Lower value make thinner heads.
* The range is limited from 0.03 to 0.4.
*/

void QwtSimpleCompassRose::setWidth(double w) 
{
   d_width = w;
   if (d_width < 0.03) 
        d_width = 0.03;

   if (d_width > 0.4) 
        d_width = 0.4;
}

void QwtSimpleCompassRose::setNumThorns(int numThorns) 
{
    if ( numThorns < 4 )
        numThorns = 4;

    if ( numThorns % 4 )
        numThorns += 4 - numThorns % 4;

    d_numThorns = numThorns;
}

int QwtSimpleCompassRose::numThorns() const
{
   return d_numThorns;
}

void QwtSimpleCompassRose::setNumThornLevels(int numThornLevels) 
{
    d_numThornLevels = numThornLevels;
}

int QwtSimpleCompassRose::numThornLevels() const
{
    return d_numThornLevels;
}
