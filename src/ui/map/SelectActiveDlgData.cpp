/**********************************************************************
--- Qt Architect generated file ---
File: SelectActiveDlgData.cpp
Last generated: Thu Sep 7 14:21:20 2000
DO NOT EDIT!!!  This file will be automatically
regenerated by qtarch.  All changes will be lost.
*********************************************************************/
#include <qt.h>
#include "SelectActiveDlgData.h"

SelectActiveDlgData::SelectActiveDlgData(QWidget *parent, const char *name)
: QDialog(parent, name, TRUE, 208)
{
	Name = new QComboBox(FALSE, this, "ComboBox_1");
	Name->setGeometry(120, 10, 330, 30);
	Name->setMinimumSize(0, 0);
	Name->setMaximumSize(32767, 32767);
	Name->setFocusPolicy(QWidget::StrongFocus);
	Name->setBackgroundMode(QWidget::NoBackground);
#if QT_VERSION < 300
	Name->setFontPropagation(QWidget::SameFont);
	Name->setPalettePropagation(QWidget::SameFont);
	#endif
	Name->setSizeLimit( 10 );
	Name->setAutoResize( FALSE );
	Name->setMaxCount( 2147483647 );
	Name->setAutoCompletion( FALSE );
	connect(Name, SIGNAL(activated(int)), SLOT(NameChanged(int)));
	connect(Name, SIGNAL(highlighted(int)), SLOT(NameChanged(int)));
	QLabel *qtarch_Label_2 = new QLabel(this, "Label_2");
	qtarch_Label_2->setGeometry(20, 10, 80, 40);
	qtarch_Label_2->setMinimumSize(0, 0);
	qtarch_Label_2->setMaximumSize(32767, 32767);
	qtarch_Label_2->setFocusPolicy(QWidget::NoFocus);
	qtarch_Label_2->setBackgroundMode(QWidget::PaletteBackground);
#if QT_VERSION < 300
	qtarch_Label_2->setFontPropagation(QWidget::SameFont);
	qtarch_Label_2->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_Label_2->setFrameStyle( 0 );
	qtarch_Label_2->setLineWidth( 1 );
	qtarch_Label_2->setMidLineWidth( 0 );
	qtarch_Label_2->QFrame::setMargin( 0 );
	qtarch_Label_2->setText( tr( "Name" ) );
	qtarch_Label_2->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_2->setMargin( 0 );
	QLabel *qtarch_Label_3 = new QLabel(this, "Label_3");
	qtarch_Label_3->setGeometry(20, 120, 70, 30);
	qtarch_Label_3->setMinimumSize(0, 0);
	qtarch_Label_3->setMaximumSize(32767, 32767);
	qtarch_Label_3->setFocusPolicy(QWidget::NoFocus);
	qtarch_Label_3->setBackgroundMode(QWidget::PaletteBackground);
#if QT_VERSION < 300
	qtarch_Label_3->setFontPropagation(QWidget::SameFont);
	qtarch_Label_3->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_Label_3->setFrameStyle( 0 );
	qtarch_Label_3->setLineWidth( 1 );
	qtarch_Label_3->setMidLineWidth( 0 );
	qtarch_Label_3->QFrame::setMargin( 0 );
	qtarch_Label_3->setText( tr( "Icon" ) );
	qtarch_Label_3->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_3->setMargin( 0 );
	IconList = new QListBox(this, "ListBox_3");
	IconList->setGeometry(120, 120, 310, 260);
	IconList->setMinimumSize(0, 0);
	IconList->setMaximumSize(32767, 32767);
	IconList->setFocusPolicy(QWidget::TabFocus);
	IconList->setBackgroundMode(QWidget::PaletteBackground);
#if QT_VERSION < 300
	IconList->setFontPropagation(QWidget::SameFont);
	IconList->setPalettePropagation(QWidget::SameFont);
	#endif
	IconList->setFrameStyle( 34 );
	IconList->setLineWidth( 2 );
	IconList->setMidLineWidth( 0 );
	IconList->QFrame::setMargin( 0 );
	IconList->setDragSelect( TRUE );
	IconList->setAutoScroll( TRUE );
	IconList->setScrollBar( TRUE );
	IconList->setAutoScrollBar( TRUE );
	IconList->setBottomScrollBar( TRUE );
	IconList->setAutoBottomScrollBar( TRUE );
	IconList->setSmoothScrolling( FALSE );
	IconList->setMultiSelection( FALSE );
	IconList->setAutoUpdate( TRUE );
	QPushButton *qtarch_PushButton_5 = new QPushButton(this, "PushButton_5");
	qtarch_PushButton_5->setGeometry(20, 400, 100, 30);
	qtarch_PushButton_5->setMinimumSize(0, 0);
	qtarch_PushButton_5->setMaximumSize(32767, 32767);
	qtarch_PushButton_5->setFocusPolicy(QWidget::TabFocus);
	qtarch_PushButton_5->setBackgroundMode(QWidget::PaletteButton);
#if QT_VERSION < 300
	qtarch_PushButton_5->setFontPropagation(QWidget::SameFont);
	qtarch_PushButton_5->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_PushButton_5->setText( tr( "Ok" ) );
	qtarch_PushButton_5->setAutoRepeat( FALSE );
	qtarch_PushButton_5->setAutoResize( FALSE );
	qtarch_PushButton_5->setToggleButton( FALSE );
	qtarch_PushButton_5->setDefault( FALSE );
	qtarch_PushButton_5->setAutoDefault( FALSE );
	qtarch_PushButton_5->setIsMenuButton( FALSE );
	connect(qtarch_PushButton_5, SIGNAL(clicked()), SLOT(accept()));
	QPushButton *qtarch_PushButton_6 = new QPushButton(this, "PushButton_6");
	qtarch_PushButton_6->setGeometry(340, 400, 100, 30);
	qtarch_PushButton_6->setMinimumSize(0, 0);
	qtarch_PushButton_6->setMaximumSize(32767, 32767);
	qtarch_PushButton_6->setFocusPolicy(QWidget::TabFocus);
	qtarch_PushButton_6->setBackgroundMode(QWidget::PaletteButton);
#if QT_VERSION < 300
	qtarch_PushButton_6->setFontPropagation(QWidget::SameFont);
	qtarch_PushButton_6->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_PushButton_6->setText( tr( "Cancel" ) );
	qtarch_PushButton_6->setAutoRepeat( FALSE );
	qtarch_PushButton_6->setAutoResize( FALSE );
	qtarch_PushButton_6->setToggleButton( FALSE );
	qtarch_PushButton_6->setDefault( FALSE );
	qtarch_PushButton_6->setAutoDefault( FALSE );
	qtarch_PushButton_6->setIsMenuButton( FALSE );
	connect(qtarch_PushButton_6, SIGNAL(clicked()), SLOT(reject()));
	QLabel *qtarch_Label_10 = new QLabel(this, "Label_10");
	qtarch_Label_10->setGeometry(20, 60, 70, 40);
	qtarch_Label_10->setMinimumSize(0, 0);
	qtarch_Label_10->setMaximumSize(32767, 32767);
	qtarch_Label_10->setFocusPolicy(QWidget::NoFocus);
	qtarch_Label_10->setBackgroundMode(QWidget::PaletteBackground);
#if QT_VERSION < 300
	qtarch_Label_10->setFontPropagation(QWidget::SameFont);
	qtarch_Label_10->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_Label_10->setFrameStyle( 0 );
	qtarch_Label_10->setLineWidth( 1 );
	qtarch_Label_10->setMidLineWidth( 0 );
	qtarch_Label_10->QFrame::setMargin( 0 );
	qtarch_Label_10->setText( tr( "Tag" ) );
	qtarch_Label_10->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_10->setMargin( 0 );
	TagList = new QComboBox(FALSE, this, "ComboBox_6");
	TagList->setGeometry(120, 60, 150, 30);
	TagList->setMinimumSize(0, 0);
	TagList->setMaximumSize(32767, 32767);
	TagList->setFocusPolicy(QWidget::StrongFocus);
	TagList->setBackgroundMode(QWidget::PaletteButton);
#if QT_VERSION < 300
	TagList->setFontPropagation(QWidget::SameFont);
	TagList->setPalettePropagation(QWidget::SameFont);
	#endif
	TagList->setSizeLimit( 10 );
	TagList->setAutoResize( FALSE );
	TagList->setMaxCount( 2147483647 );
	TagList->setAutoCompletion( FALSE );
	resize(460,450);
	setMinimumSize(0, 0);
	setMaximumSize(32767, 32767);
}
SelectActiveDlgData::~SelectActiveDlgData()
{
}
