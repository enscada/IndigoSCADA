/**********************************************************************
--- Qt Architect generated file ---
File: Rfc1006driverConfigurationData.cpp
Last generated: Thu Jan 4 16:13:32 2001
DO NOT EDIT!!!  This file will be automatically
regenerated by qtarch.  All changes will be lost.
*********************************************************************/
#include <qt.h>
#include "Rfc1006driverConfigurationData.h"
Rfc1006driverConfigurationData::Rfc1006driverConfigurationData(QWidget *parent, const char *name)
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

	RFC1006ServerIPAddressText = new QLineEdit(this, "LineEdit_7");
	RFC1006ServerIPAddressText->setGeometry(200, 40, 100, 30);
	RFC1006ServerIPAddressText->setMinimumSize(0, 0);
	RFC1006ServerIPAddressText->setMaximumSize(32767, 32767);
	RFC1006ServerIPAddressText->setFocusPolicy(QWidget::StrongFocus);
	RFC1006ServerIPAddressText->setBackgroundMode(QWidget::PaletteBase);
#if QT_VERSION < 300
	RFC1006ServerIPAddressText->setFontPropagation(QWidget::SameFont);
	RFC1006ServerIPAddressText->setPalettePropagation(QWidget::SameFont);
	#endif
	RFC1006ServerIPAddressText->setText( tr( "" ) );
	RFC1006ServerIPAddressText->setMaxLength( 100 );
	RFC1006ServerIPAddressText->setFrame( QLineEdit::Normal );
	RFC1006ServerIPAddressText->setFrame( TRUE );
	RFC1006ServerIPAddressText->setAlignment( AlignLeft );
	QLabel *qtarch_Label_22 = new QLabel(this, "Label_22");
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
	qtarch_Label_22->setText( tr( "RFC1006 slave IP address" ) );
	qtarch_Label_22->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_22->setMargin( 0 );


	RFC1006ServerIPPortText = new QLineEdit(this, "LineEdit_7");
	RFC1006ServerIPPortText->setGeometry(200, 80, 100, 30);
	RFC1006ServerIPPortText->setMinimumSize(0, 0);
	RFC1006ServerIPPortText->setMaximumSize(32767, 32767);
	RFC1006ServerIPPortText->setFocusPolicy(QWidget::StrongFocus);
	RFC1006ServerIPPortText->setBackgroundMode(QWidget::PaletteBase);
#if QT_VERSION < 300
	RFC1006ServerIPPortText->setFontPropagation(QWidget::SameFont);
	RFC1006ServerIPPortText->setPalettePropagation(QWidget::SameFont);
	#endif
	RFC1006ServerIPPortText->setText( tr( "" ) );
	RFC1006ServerIPPortText->setMaxLength( 100 );
	RFC1006ServerIPPortText->setFrame( QLineEdit::Normal );
	RFC1006ServerIPPortText->setFrame( TRUE );
	RFC1006ServerIPPortText->setAlignment( AlignLeft );
	QLabel *qtarch_Label_23 = new QLabel(this, "Label_23");
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
	qtarch_Label_23->setText( tr( "RFC1006 slave TCP port" ) );
	qtarch_Label_23->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_23->setMargin( 0 );

	RFC1006ServerSlotText = new QLineEdit(this, "LineEdit_7");
	RFC1006ServerSlotText->setGeometry(200, 120, 100, 30);
	RFC1006ServerSlotText->setMinimumSize(0, 0);
	RFC1006ServerSlotText->setMaximumSize(32767, 32767);
	RFC1006ServerSlotText->setFocusPolicy(QWidget::StrongFocus);
	RFC1006ServerSlotText->setBackgroundMode(QWidget::PaletteBase);
#if QT_VERSION < 300
	RFC1006ServerSlotText->setFontPropagation(QWidget::SameFont);
	RFC1006ServerSlotText->setPalettePropagation(QWidget::SameFont);
	#endif
	RFC1006ServerSlotText->setText( tr( "" ) );
	RFC1006ServerSlotText->setMaxLength( 100 );
	RFC1006ServerSlotText->setFrame( QLineEdit::Normal );
	RFC1006ServerSlotText->setFrame( TRUE );
	RFC1006ServerSlotText->setAlignment( AlignLeft );
	QLabel *qtarch_Label_24 = new QLabel(this, "Label_24");
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
	qtarch_Label_24->setText( tr( "CP 343 or CP 443 slot" ) );
	qtarch_Label_24->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_24->setMargin( 0 );


	PLCAddressText = new QLineEdit(this, "LineEdit_7");
	PLCAddressText->setGeometry(200, 160, 100, 30);
	PLCAddressText->setMinimumSize(0, 0);
	PLCAddressText->setMaximumSize(32767, 32767);
	PLCAddressText->setFocusPolicy(QWidget::StrongFocus);
	PLCAddressText->setBackgroundMode(QWidget::PaletteBase);
#if QT_VERSION < 300
	PLCAddressText->setFontPropagation(QWidget::SameFont);
	PLCAddressText->setPalettePropagation(QWidget::SameFont);
	#endif
	PLCAddressText->setText( tr( "" ) );
	PLCAddressText->setMaxLength( 100 );
	PLCAddressText->setFrame( QLineEdit::Normal );
	PLCAddressText->setFrame( TRUE );
	PLCAddressText->setAlignment( AlignLeft );
	QLabel *qtarch_Label_25 = new QLabel(this, "Label_25");
	qtarch_Label_25->setGeometry(10, 160, 150, 30);
	qtarch_Label_25->setMinimumSize(0, 0);
	qtarch_Label_25->setMaximumSize(32767, 32767);
	qtarch_Label_25->setFocusPolicy(QWidget::NoFocus);
	qtarch_Label_25->setBackgroundMode(QWidget::PaletteBackground);
	#if QT_VERSION < 300
	qtarch_Label_25->setFontPropagation(QWidget::SameFont);
	qtarch_Label_25->setPalettePropagation(QWidget::SameFont);
	#endif
	qtarch_Label_25->setFrameStyle( 0 );
	qtarch_Label_25->setLineWidth( 1 );
	qtarch_Label_25->setMidLineWidth( 0 );
	qtarch_Label_25->QFrame::setMargin( 0 );
	qtarch_Label_25->setText( tr( "PLC address" ) );
	qtarch_Label_25->setAlignment( AlignLeft|AlignVCenter|ExpandTabs );
	qtarch_Label_25->setMargin( 0 );

/////////////////////////////////////////////////////////////////////////
	
/////////////////////////////////////////////////////////////////////////
	QLabel *qtarch_Label_10 = new QLabel(this, "Label_10");
	qtarch_Label_10->setGeometry(10, 200, 100, 30);
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
	NItems->setGeometry(200, 200, 100, 30);
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
Rfc1006driverConfigurationData::~Rfc1006driverConfigurationData()
{
}
