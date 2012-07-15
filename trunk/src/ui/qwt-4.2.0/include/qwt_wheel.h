/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_WHEEL_H
#define QWT_WHEEL_H

#include "qwt_global.h"
#include "qwt_sldbase.h"

/*!
  \brief The Wheel Widget

  The wheel widget can be used to change values over a very large range
  in very small steps. Using the setMass member, it can be configured
  as a flywheel.

  \sa The radio example.
*/
class QWT_EXPORT QwtWheel : public QwtSliderBase
{
    Q_OBJECT 
    Q_PROPERTY( double totalAngle READ totalAngle WRITE setTotalAngle )
    Q_PROPERTY( double viewAngle READ viewAngle WRITE setViewAngle )
    Q_PROPERTY( int    tickCnt READ tickCnt WRITE setTickCnt )
    Q_PROPERTY( int    internalBorder READ internalBorder WRITE setInternalBorder )
    Q_PROPERTY( double mass READ mass WRITE setMass )
            
public:
    QwtWheel(QWidget *parent = 0, const char *name = 0);
    virtual ~QwtWheel();

    virtual void setOrientation(Qt::Orientation);

    double totalAngle() const {return d_totalAngle;}
        double viewAngle() const {return d_viewAngle;}
    int tickCnt() const {return d_tickCnt;}
    int internalBorder() const {return d_intBorder;}
        double mass() const;

    void setTotalAngle (double angle);
    void setTickCnt(int cnt);
    void setViewAngle(double angle);
    void setInternalBorder(int width);
    void setMass(double val);
    void setWheelWidth( int w );

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;
    virtual QSizePolicy sizePolicy() const;

protected:
    virtual void resizeEvent(QResizeEvent *e);
    virtual void paintEvent(QPaintEvent *e);

    void layoutWheel( bool update = TRUE );
    void draw(QPainter *p, const QRect& update_rect);
    void drawWheel(QPainter *p, const QRect &r);
    void drawWheelBackground(QPainter *p, const QRect &r);
    void setColorArray();

    virtual void valueChange();
    virtual void paletteChange( const QPalette &);

    virtual double getValue(const QPoint &p);
    virtual void getScrollMode(const QPoint &p, 
        int &scrollMode, int &direction);

private:
    QColor* d_colors;
    QRect d_sliderRect;
    double d_viewAngle;
    double d_totalAngle;
    int d_tickCnt;
    int d_intBorder;
    int d_borderWidth;
    int d_colorCnt;
    int d_wheelWidth;
    int d_allocContext;
};

#endif
