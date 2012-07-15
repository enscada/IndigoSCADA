/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

#ifndef QWT_SYMBOL_H
#define QWT_SYMBOL_H

#include <qbrush.h>
#include <qpen.h>
#include <qsize.h>
#include "qwt_global.h"
#include "qwt.h"

class QPainter;

//! A class for drawing symbols
class QWT_EXPORT QwtSymbol
{
public:
    /*!
        Style
        \sa QwtSymbol::setStyle, QwtSymbol::style
     */
    enum Style { None, Ellipse, Rect, Diamond, Triangle, DTriangle,
        UTriangle, LTriangle, RTriangle, Cross, XCross, StyleCnt }; 
   
public:
    QwtSymbol();
    QwtSymbol(Style st, const QBrush &bd, const QPen &pn, const QSize &s);
    virtual ~QwtSymbol();
    
    bool operator!=(const QwtSymbol &) const;
    bool operator==(const QwtSymbol &) const;

    void setSize(const QSize &s);
    void setSize(int a, int b = -1);
    void setBrush(const QBrush& b);
    void setPen(const QPen &p);
    void setStyle (Style s);

    //! Return Brush
    const QBrush& brush() const { return d_brush; }
    //! Return Pen
    const QPen& pen() const { return d_pen; }
    //! Return Size
    const QSize& size() const { return d_size; }
    //! Return Style
    Style style() const { return d_style; } 
    
    void draw(QPainter *p, const QPoint &pt) const; 
    void draw(QPainter *p, int x, int y) const;
    virtual void draw(QPainter *p, const QRect &r) const;

private:
    QBrush d_brush;
    QPen d_pen;
    QSize d_size;
    Style d_style;
};

#endif
