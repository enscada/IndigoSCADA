/**********************************************************************
--- Qt Architect generated file ---
File: CalculatedInputData.cpp
Last generated: Tue Sep 12 13:53:13 2000
DO NOT EDIT!!!  This file will be automatically
regenerated by qtarch.  All changes will be lost.
*********************************************************************/
#include <qpixmap.h>
#include <qlayout.h>
#include "CalculatedInputData.h"
#include <qpushbutton.h>
CalculatedInputData::CalculatedInputData(QWidget *parent, const char *name)
: QFrame(parent, name, 16)
{
	/*
	QPushButton *qtarch_PushButton_4 = new QPushButton(this, "PushButton_4");
	qtarch_PushButton_4->setGeometry(330, 50, 100, 30);
	qtarch_PushButton_4->setMinimumSize(0, 0);
	qtarch_PushButton_4->setMaximumSize(32767, 32767);
	qtarch_PushButton_4->setFocusPolicy(QWidget::TabFocus);
	qtarch_PushButton_4->setBackgroundMode(QWidget::PaletteButton);
	#if QT_VERSION < 300
	qtarch_PushButton_4->setFontPropagation(QWidget::SameFont);
	qtarch_PushButton_4->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_PushButton_4->setText( tr( "Expression..." ) );
	qtarch_PushButton_4->setAutoRepeat( FALSE );
	qtarch_PushButton_4->setAutoResize( FALSE );
	qtarch_PushButton_4->setToggleButton( FALSE );
	qtarch_PushButton_4->setDefault( FALSE );
	qtarch_PushButton_4->setAutoDefault( FALSE );
	qtarch_PushButton_4->setIsMenuButton( FALSE );
	connect(qtarch_PushButton_4, SIGNAL(clicked()), SLOT(Expression()));
	*/	
	Expr = new QLabel(this, "Label_5");
	Expr->setGeometry(10, 10, 420, 30);
	Expr->setMinimumSize(0, 0);
	Expr->setMaximumSize(32767, 32767);
	Expr->setFocusPolicy(QWidget::NoFocus);
	Expr->setBackgroundMode(QWidget::PaletteBackground);
	#if QT_VERSION < 300
	Expr->setFontPropagation(QWidget::SameFont);
	Expr->setPalettePropagation(QWidget::SameFont);
	#endif
	Expr->setFrameStyle( 49 );
	Expr->setLineWidth( 1 );
	Expr->setMidLineWidth( 0 );
	Expr->QFrame::setMargin( 0 );
	Expr->setText( "" );
	Expr->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	Expr->setMargin( 0 );

	resize(440,90);
	setMinimumSize(0, 0);
	setMaximumSize(32767, 32767);
}
CalculatedInputData::~CalculatedInputData()
{
}

