/* -*- mode: C++ ; c-file-style: "stroustrup" -*- *****************************
 * Qwt Widget Library
 * Copyright (C) 1997   Josef Wilgen
 * Copyright (C) 2002   Uwe Rathmann
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the Qwt License, Version 1.0
 *****************************************************************************/

// vim: expandtab

#ifndef QWT_DIAL_H
#define QWT_DIAL_H 1

#include <qframe.h>
#include <qpalette.h>
#include "qwt_global.h"
#include "qwt_sldbase.h"
#include "qwt_scldraw.h"

class QwtDialNeedle;
class QwtDial;

/*!
  A special scale draw made for QwtDial
  
  \sa QwtDial, QwtCompass
*/

class QWT_EXPORT QwtDialScaleDraw: public QwtScaleDraw
{
public:
    QwtDialScaleDraw(QwtDial *);
    virtual QString label(double value) const;

    void showLabels(bool);
    bool visibleLabels() const;

    void setPenWidth(uint);
    uint penWidth() const;

private:
    QwtDial *d_parent;
    int d_penWidth;
    bool d_visibleLabels;
};

/*!
  \brief QwtDial class provides a rounded range control. 

  QwtDial is intended as base class for dial widgets like
  speedometers, compass widgets, clocks ... 

  \image html dial.gif

  A dial contains a scale and a needle indicating the current value
  of the dial. Depending on Mode one of them is fixed and the 
  other is rotating. If not isReadOnly() the
  dial can be rotated by dragging the mouse or using keyboard inputs 
  (see keyPressEvent()). A dial might be wrapping, what means
  a rotation below/above one limit continues on the other limit (f.e compass).
  The scale might cover any arc of the dial, its values are related to
  the origin() of the dial.
  
  Qwt is missing a set of good looking needles (QwtDialNeedle).
  Contributions are very welcome.
  
  \sa QwtCompass, QwtAnalogClock, QwtDialNeedle
  \note The examples/dials example shows different types of dials.
*/

class QWT_EXPORT QwtDial: public QwtSliderBase
{
    Q_OBJECT

    Q_ENUMS(Shadow)
    Q_ENUMS(Mode)

    Q_PROPERTY(bool visibleBackground READ hasVisibleBackground WRITE showBackground)
    Q_PROPERTY(int lineWidth READ lineWidth WRITE setLineWidth)
    Q_PROPERTY(Shadow frameShadow READ frameShadow WRITE setFrameShadow)
    Q_PROPERTY(Mode mode READ mode WRITE setMode)
    Q_PROPERTY(double origin READ origin WRITE setOrigin)
    Q_PROPERTY(bool wrapping READ wrapping WRITE setWrapping)

    friend class QwtDialScaleDraw;
public:

    /*!
        \brief Frame shadow

         Unfortunately it is not possible to use QFrame::Shadow
         as a property of a widget that is not derived from QFrame.
         The following enum is made for the designer only. It is safe
         to use QFrame::Shadow instead.
     */
    enum Shadow
    {
        Plain = QFrame::Plain,
        Raised = QFrame::Raised,
        Sunken = QFrame::Sunken
    };

    //! see QwtDial::setScaleOptions
    enum ScaleOptions
    {
        ScaleBackbone = 1,
        ScaleTicks = 2,
        ScaleLabel = 4
    };

    /*!
        In case of RotateNeedle the needle is rotating, in case of
        RotateScale, the needle points to origin()
        and the scale is rotating.
    */
    enum Mode
    {
        RotateNeedle,
        RotateScale
    };

    QwtDial( QWidget* parent=0, const char* name = 0);
    virtual ~QwtDial();

    void setFrameShadow(Shadow);
    Shadow frameShadow() const;

    bool hasVisibleBackground() const;
    void showBackground(bool);

    void setLineWidth(int);
    int lineWidth() const;

    void setMode(Mode);
    Mode mode() const;

    virtual void setWrapping(bool);
    bool wrapping() const;

    virtual void setScale(int maxMajIntv, int maxMinIntv, double step = 0.0);

    void setScaleArc(double min, double max);
    void setScaleOptions(int);
    void setScaleTicks(int minLen, int medLen, int majLen, int penWidth = 1);

    //! Return the lower limit of the scale arc
    double minScaleArc() const { return d_minScaleArc; }
    //! Return the upper limit of the scale arc
    double maxScaleArc() const { return d_maxScaleArc; }

    virtual void setOrigin(double);
    double origin() const;

    virtual void setNeedle(QwtDialNeedle *);
    const QwtDialNeedle *needle() const;
    QwtDialNeedle *needle();

    QRect boundingRect() const;
    QRect contentsRect() const;
    virtual QRect scaleContentsRect() const;

    virtual QSize sizeHint() const;
    virtual QSize minimumSizeHint() const;

    virtual void setScaleDraw(QwtDialScaleDraw *);

    //! Return the scale draw
    QwtDialScaleDraw *scaleDraw() { return d_scaleDraw; }
    //! Return the scale draw
    const QwtDialScaleDraw *scaleDraw() const { return d_scaleDraw; }

protected:
    virtual void paintEvent(QPaintEvent *);
    virtual void resizeEvent(QResizeEvent *);
    virtual void keyPressEvent(QKeyEvent *);

    virtual void drawFrame(QPainter *p);
    virtual void drawContents(QPainter *) const;
    virtual void drawFocusIndicator(QPainter *) const;

    virtual void drawScale(QPainter *, const QPoint &center,
        int radius, double origin, double arcMin, double arcMax) const;

    /*!
      Draw the contents inside the scale

      Paints nothing.

      \param painter Painter
      \param center Center of the contents circle
      \param radius Radius of the contents circle
    */
    virtual void drawScaleContents(QPainter *painter, const QPoint &center, 
        int radius) const;

    virtual void drawNeedle(QPainter *, const QPoint &, 
        int radius, double direction, QPalette::ColorGroup) const;

    virtual QString scaleLabel(double) const;
    void updateScale();

    virtual void rangeChange();
    virtual void valueChange();

    virtual double getValue(const QPoint &);
    virtual void getScrollMode(const QPoint &, 
        int &scrollMode, int &direction);

private:
    bool d_visibleBackground;
    Shadow d_frameShadow;
    int d_lineWidth;

    Mode d_mode;

    double d_origin;
    double d_minScaleArc;
    double d_maxScaleArc;

    QwtDialScaleDraw *d_scaleDraw;
    int d_maxMajIntv;
    int d_maxMinIntv;
    double d_scaleStep;

    QwtDialNeedle *d_needle;

    static double d_previousDir;
};

#endif
