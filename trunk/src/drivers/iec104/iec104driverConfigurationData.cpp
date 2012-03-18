/**********************************************************************
--- Qt Architect generated file ---
File: Iec104driverConfigurationData.cpp
Last generated: Thu Jan 4 16:13:32 2001
DO NOT EDIT!!!  This file will be automatically
regenerated by qtarch.  All changes will be lost.
*********************************************************************/
#include <qt.h>
#include "Iec104driverConfigurationData.h"
Iec104driverConfigurationData::Iec104driverConfigurationData(QWidget *parent, const char *name)
: QDialog(parent, name, TRUE, 208)
{
	QLabel *qtarch_Label_7 = new QLabel(this, "Label_7");
	qtarch_Label_7->setGeometry(10, 0, 90, 30);
	qtarch_Label_7->setMinimumSize(0, 0);
	qtarch_Label_7->setMaximumSize(32767, 32767);
	qtarch_Label_7->setFocusPolicy(QWidget::NoFocus);
	qtarch_Label_7->setBackgroundMode(QWidget::PaletteBackground);
	#if QT_VERSION < 300
	qtarch_Label_7->setFontPropagation(QWidget::SameFont);
	qtarch_Label_7->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_Label_7->setFrameStyle( 0 );
	qtarch_Label_7->setLineWidth( 1 );
	qtarch_Label_7->setMidLineWidth( 0 );
	qtarch_Label_7->QFrame::setMargin( 0 );
	qtarch_Label_7->setText( tr( "Name" ) );
	qtarch_Label_7->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_7->setMargin( 0 );
	Name = new QLabel(this, "Label_8");
	Name->setGeometry(200, 0, 250, 30);
	Name->setMinimumSize(0, 0);
	Name->setMaximumSize(32767, 32767);
	Name->setFocusPolicy(QWidget::NoFocus);
	Name->setBackgroundMode(QWidget::PaletteBackground);
	#if QT_VERSION < 300
	Name->setFontPropagation(QWidget::SameFont);
	Name->setPalettePropagation(QWidget::SameFont);
	#endif
	Name->setFrameStyle( 50 );
	Name->setLineWidth( 1 );
	Name->setMidLineWidth( 0 );
	Name->QFrame::setMargin( 0 );
	Name->setText( "" );
	Name->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	Name->setMargin( 0 );

	IEC104ServerIPAddressText = new QLineEdit(this, "LineEdit_7");
	IEC104ServerIPAddressText->setGeometry(200, 40, 100, 30);
	IEC104ServerIPAddressText->setMinimumSize(0, 0);
	IEC104ServerIPAddressText->setMaximumSize(32767, 32767);
	IEC104ServerIPAddressText->setFocusPolicy(QWidget::StrongFocus);
	IEC104ServerIPAddressText->setBackgroundMode(QWidget::PaletteBase);
#if QT_VERSION < 300
	IEC104ServerIPAddressText->setFontPropagation(QWidget::SameFont);
	IEC104ServerIPAddressText->setPalettePropagation(QWidget::SameFont);
	#endif
	IEC104ServerIPAddressText->setText( tr( "" ) );
	IEC104ServerIPAddressText->setMaxLength( 100 );
	IEC104ServerIPAddressText->setFrame( QLineEdit::Normal );
	IEC104ServerIPAddressText->setFrame( TRUE );
	IEC104ServerIPAddressText->setAlignment( AlignLeft );
	QLabel *qtarch_Label_22 = new QLabel(this, "Label_21");
	qtarch_Label_22->setGeometry(10, 40, 150, 30);
	qtarch_Label_22->setMinimumSize(0, 0);
	qtarch_Label_22->setMaximumSize(32767, 32767);
	qtarch_Label_22->setFocusPolicy(QWidget::NoFocus);
	qtarch_Label_22->setBackgroundMode(QWidget::PaletteBackground);
	#if QT_VERSION < 300
	qtarch_Label_22->setFontPropagation(QWidget::SameFont);
	qtarch_Label_22->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_Label_22->setFrameStyle( 0 );
	qtarch_Label_22->setLineWidth( 1 );
	qtarch_Label_22->setMidLineWidth( 0 );
	qtarch_Label_22->QFrame::setMargin( 0 );
	qtarch_Label_22->setText( tr( "IEC 104 slave IP address" ) );
	qtarch_Label_22->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_22->setMargin( 0 );


	IEC104ServerIPPortText = new QLineEdit(this, "LineEdit_7");
	IEC104ServerIPPortText->setGeometry(200, 80, 100, 30);
	IEC104ServerIPPortText->setMinimumSize(0, 0);
	IEC104ServerIPPortText->setMaximumSize(32767, 32767);
	IEC104ServerIPPortText->setFocusPolicy(QWidget::StrongFocus);
	IEC104ServerIPPortText->setBackgroundMode(QWidget::PaletteBase);
#if QT_VERSION < 300
	IEC104ServerIPPortText->setFontPropagation(QWidget::SameFont);
	IEC104ServerIPPortText->setPalettePropagation(QWidget::SameFont);
	#endif
	IEC104ServerIPPortText->setText( tr( "" ) );
	IEC104ServerIPPortText->setMaxLength( 100 );
	IEC104ServerIPPortText->setFrame( QLineEdit::Normal );
	IEC104ServerIPPortText->setFrame( TRUE );
	IEC104ServerIPPortText->setAlignment( AlignLeft );
	QLabel *qtarch_Label_23 = new QLabel(this, "Label_21");
	qtarch_Label_23->setGeometry(10, 80, 150, 30);
	qtarch_Label_23->setMinimumSize(0, 0);
	qtarch_Label_23->setMaximumSize(32767, 32767);
	qtarch_Label_23->setFocusPolicy(QWidget::NoFocus);
	qtarch_Label_23->setBackgroundMode(QWidget::PaletteBackground);
	#if QT_VERSION < 300
	qtarch_Label_23->setFontPropagation(QWidget::SameFont);
	qtarch_Label_23->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_Label_23->setFrameStyle( 0 );
	qtarch_Label_23->setLineWidth( 1 );
	qtarch_Label_23->setMidLineWidth( 0 );
	qtarch_Label_23->QFrame::setMargin( 0 );
	qtarch_Label_23->setText( tr( "IEC 104 slave TCP port" ) );
	qtarch_Label_23->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_23->setMargin( 0 );


	IEC104ServerCASDUText = new QLineEdit(this, "LineEdit_7");
	IEC104ServerCASDUText->setGeometry(200, 120, 100, 30);
	IEC104ServerCASDUText->setMinimumSize(0, 0);
	IEC104ServerCASDUText->setMaximumSize(32767, 32767);
	IEC104ServerCASDUText->setFocusPolicy(QWidget::StrongFocus);
	IEC104ServerCASDUText->setBackgroundMode(QWidget::PaletteBase);
#if QT_VERSION < 300
	IEC104ServerCASDUText->setFontPropagation(QWidget::SameFont);
	IEC104ServerCASDUText->setPalettePropagation(QWidget::SameFont);
	#endif
	IEC104ServerCASDUText->setText( tr( "" ) );
	IEC104ServerCASDUText->setMaxLength( 100 );
	IEC104ServerCASDUText->setFrame( QLineEdit::Normal );
	IEC104ServerCASDUText->setFrame( TRUE );
	IEC104ServerCASDUText->setAlignment( AlignLeft );
	QLabel *qtarch_Label_24 = new QLabel(this, "Label_21");
	qtarch_Label_24->setGeometry(10, 120, 150, 30);
	qtarch_Label_24->setMinimumSize(0, 0);
	qtarch_Label_24->setMaximumSize(32767, 32767);
	qtarch_Label_24->setFocusPolicy(QWidget::NoFocus);
	qtarch_Label_24->setBackgroundMode(QWidget::PaletteBackground);
	#if QT_VERSION < 300
	qtarch_Label_24->setFontPropagation(QWidget::SameFont);
	qtarch_Label_24->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_Label_24->setFrameStyle( 0 );
	qtarch_Label_24->setLineWidth( 1 );
	qtarch_Label_24->setMidLineWidth( 0 );
	qtarch_Label_24->QFrame::setMargin( 0 );
	qtarch_Label_24->setText( tr( "IEC 104 slave CASDU" ) );
	qtarch_Label_24->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_24->setMargin( 0 );
/////////////////////////////////////////////////////////////////////////
	
/////////////////////////////////////////////////////////////////////////
	QLabel *qtarch_Label_10 = new QLabel(this, "Label_10");
	qtarch_Label_10->setGeometry(10, 160, 100, 30);
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
	qtarch_Label_10->setText( tr( "N Items" ) );
	qtarch_Label_10->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_10->setMargin( 0 );

	NItems = new QSpinBox(this, "SpinBox_3");
	NItems->setGeometry(200, 160, 100, 30);
	NItems->setMinimumSize(0, 0);
	NItems->setMaximumSize(32767, 32767);
	NItems->setFocusPolicy(QWidget::StrongFocus);
	NItems->setBackgroundMode(QWidget::PaletteBackground);
	#if QT_VERSION < 300
	NItems->setFontPropagation(QWidget::SameFont);
	NItems->setPalettePropagation(QWidget::SameFont);
	NItems->setFrameStyle( 50 );
	NItems->setLineWidth( 2 );
	NItems->setMidLineWidth( 0 );
	NItems->QFrame::setMargin( 0 );
	#endif
	NItems->setRange(0, 5000);
	NItems->setSteps(1, 0);
	NItems->setPrefix( "" );
	NItems->setSuffix( "" );
	NItems->setSpecialValueText( "" );
	NItems->setWrapping( FALSE );
/////////////////////////////////////////////////////////////////////////////////
	
	QPushButton *qtarch_PushButton_1 = new QPushButton(this, "PushButton_1");
	qtarch_PushButton_1->setGeometry(20, 230, 100, 30);
	qtarch_PushButton_1->setMinimumSize(0, 0);
	qtarch_PushButton_1->setMaximumSize(32767, 32767);
	qtarch_PushButton_1->setFocusPolicy(QWidget::TabFocus);
	qtarch_PushButton_1->setBackgroundMode(QWidget::PaletteButton);
	#if QT_VERSION < 300
	qtarch_PushButton_1->setFontPropagation(QWidget::SameFont);
	qtarch_PushButton_1->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_PushButton_1->setText( tr( "Ok" ) );
	qtarch_PushButton_1->setAutoRepeat( FALSE );
	qtarch_PushButton_1->setAutoResize( FALSE );
	qtarch_PushButton_1->setToggleButton( FALSE );
	qtarch_PushButton_1->setDefault( FALSE );
	qtarch_PushButton_1->setAutoDefault( FALSE );
	qtarch_PushButton_1->setIsMenuButton( FALSE );
	connect(qtarch_PushButton_1, SIGNAL(clicked()), SLOT(OkClicked()));
	QPushButton *qtarch_PushButton_2 = new QPushButton(this, "PushButton_2");
	qtarch_PushButton_2->setGeometry(210, 230, 100, 30);
	qtarch_PushButton_2->setMinimumSize(0, 0);
	qtarch_PushButton_2->setMaximumSize(32767, 32767);
	qtarch_PushButton_2->setFocusPolicy(QWidget::TabFocus);
	qtarch_PushButton_2->setBackgroundMode(QWidget::PaletteButton);
	#if QT_VERSION < 300
	qtarch_PushButton_2->setFontPropagation(QWidget::SameFont);
	qtarch_PushButton_2->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_PushButton_2->setText( tr( "&Help" ) );
	qtarch_PushButton_2->setAutoRepeat( FALSE );
	qtarch_PushButton_2->setAutoResize( FALSE );
	qtarch_PushButton_2->setToggleButton( FALSE );
	qtarch_PushButton_2->setDefault( FALSE );
	qtarch_PushButton_2->setAutoDefault( FALSE );
	qtarch_PushButton_2->setIsMenuButton( FALSE );
	connect(qtarch_PushButton_2, SIGNAL(clicked()), SLOT(Help()));
	QPushButton *qtarch_PushButton_3 = new QPushButton(this, "PushButton_3");
	qtarch_PushButton_3->setGeometry(360, 230, 100, 30);
	qtarch_PushButton_3->setMinimumSize(0, 0);
	qtarch_PushButton_3->setMaximumSize(32767, 32767);
	qtarch_PushButton_3->setFocusPolicy(QWidget::TabFocus);
	qtarch_PushButton_3->setBackgroundMode(QWidget::PaletteButton);
	#if QT_VERSION < 300
	qtarch_PushButton_3->setFontPropagation(QWidget::SameFont);
	qtarch_PushButton_3->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_PushButton_3->setText( tr( "Cancel" ) );
	qtarch_PushButton_3->setAutoRepeat( FALSE );
	qtarch_PushButton_3->setAutoResize( FALSE );
	qtarch_PushButton_3->setToggleButton( FALSE );
	qtarch_PushButton_3->setDefault( FALSE );
	qtarch_PushButton_3->setAutoDefault( FALSE );
	qtarch_PushButton_3->setIsMenuButton( FALSE );
	connect(qtarch_PushButton_3, SIGNAL(clicked()), SLOT(reject()));

	resize(480,280);
	setMinimumSize(0, 0);
	setMaximumSize(32767, 32767);
}
Iec104driverConfigurationData::~Iec104driverConfigurationData()
{
}

