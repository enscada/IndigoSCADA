/**********************************************************************
** Copyright (C) 2000-2007 Trolltech ASA.  All rights reserved.
**
** This file is part of Qt Designer.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include <qvariant.h> // HP-UX compiler needs this here

#include "pixmapchooser.h"
#include "formwindow.h"
#if defined(DESIGNER) && !defined(RESOURCE)
#include "pixmapfunction.h"
#endif
#include "metadatabase.h"
#include "mainwindow.h"
#include "pixmapcollectioneditor.h"
#include "pixmapcollection.h"
#include "project.h"

#include <qapplication.h>
#include <qimage.h>
#include <qpainter.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qiconview.h>

#if defined(DESIGNER)
static ImageIconProvider *imageIconProvider = 0;
#endif

PixmapView::PixmapView( QWidget *parent )
    : QScrollView( parent )
{
    viewport()->setBackgroundMode( PaletteBase );
}

void PixmapView::setPixmap( const QPixmap &pix )
{
    pixmap = pix;
    resizeContents( pixmap.size().width(), pixmap.size().height() );
    viewport()->repaint( FALSE );
}

void PixmapView::drawContents( QPainter *p, int cx, int cy, int cw, int ch )
{
    p->fillRect( cx, cy, cw, ch, colorGroup().brush( QColorGroup::Base ) );
    p->drawPixmap( 0, 0, pixmap );
}

void PixmapView::previewUrl( const QUrl &u )
{
    if ( u.isLocalFile() ) {
	QString path = u.path();
	QPixmap pix( path );
	if ( !pix.isNull() )
	    setPixmap( pix );
    } else {
	qWarning( "Previewing remote files not supported." );
    }
}

static void buildImageFormatList( QString &filter, QString &all )
{
    all = qApp->translate( "qChoosePixmap", "All Pixmaps (" );
    for ( uint i = 0; i < QImageIO::outputFormats().count(); i++ ) {
	QString outputFormat = QImageIO::outputFormats().at( i );
	QString outputExtension;
	if ( outputFormat != "JPEG" )
	   outputExtension = outputFormat.lower();
	else
	    outputExtension = "jpg;*.jpeg";
	filter += qApp->translate( "qChoosePixmap", "%1-Pixmaps (%2)\n" ).
		  arg( outputFormat ).arg( "*." + outputExtension);
	all += "*." + outputExtension + ";";
    }
    filter.prepend( all + qApp->translate( "qChoosePixmap", ")\n" ) );
    filter += qApp->translate( "qChoosePixmap", "All Files (*)" );

}
QStringList qChoosePixmaps( QWidget *parent )
{
    if ( !imageIconProvider && !QFileDialog::iconProvider() )
	QFileDialog::setIconProvider( ( imageIconProvider = new ImageIconProvider ) );

    QString filter;
    QString all;
    buildImageFormatList( filter, all );

    QFileDialog fd( QString::null, filter, parent, 0, TRUE );
    fd.setMode( QFileDialog::ExistingFiles );
    fd.setContentsPreviewEnabled( TRUE );
    PixmapView *pw = new PixmapView( &fd );
    fd.setContentsPreview( pw, pw );
    fd.setViewMode( QFileDialog::List );
    fd.setPreviewMode( QFileDialog::Contents );
    fd.setCaption( qApp->translate( "qChoosePixmap", "Choose Images..." ) );
    if ( fd.exec() == QDialog::Accepted )
	return fd.selectedFiles();
    return QStringList();
}

QPixmap qChoosePixmap( QWidget *parent, FormWindow *fw, const QPixmap &old, QString *fn )
{
#if defined(DESIGNER)
    if ( !fw || fw->savePixmapInline() ) {
	if ( !imageIconProvider && !QFileDialog::iconProvider() )
	    QFileDialog::setIconProvider( ( imageIconProvider = new ImageIconProvider ) );

	QString filter;
	QString all;
	buildImageFormatList( filter, all );

	QFileDialog fd( QString::null, filter, parent, 0, TRUE );
	fd.setContentsPreviewEnabled( TRUE );
	PixmapView *pw = new PixmapView( &fd );
	fd.setContentsPreview( pw, pw );
	fd.setViewMode( QFileDialog::List );
	fd.setPreviewMode( QFileDialog::Contents );
	fd.setCaption( qApp->translate( "qChoosePixmap", "Choose a Pixmap..." ) );
	if ( fd.exec() == QDialog::Accepted ) {
	    QPixmap pix( fd.selectedFile() );
	    if ( fn )
		*fn = fd.selectedFile();
	    MetaDataBase::setPixmapArgument( fw, pix.serialNumber(), fd.selectedFile() );
	    return pix;
	}
    } else if ( fw && fw->savePixmapInProject() ) {
	PixmapCollectionEditor dia( parent, 0, TRUE );
	dia.setProject( fw->project() );
	dia.setChooserMode( TRUE );
	dia.setCurrentItem( MetaDataBase::pixmapKey( fw, old.serialNumber() ) );
	if ( dia.exec() == QDialog::Accepted ) {
	    QPixmap pix( fw->project()->pixmapCollection()->pixmap( dia.viewPixmaps->currentItem()->text() ) );
	    MetaDataBase::setPixmapKey( fw, pix.serialNumber(), dia.viewPixmaps->currentItem()->text() );
	    return pix;
	}
    } else {
	PixmapFunction dia( parent, 0, TRUE );
	QObject::connect( dia.helpButton, SIGNAL( clicked() ), MainWindow::self, SLOT( showDialogHelp() ) );
	dia.labelFunction->setText( fw->pixmapLoaderFunction() + "(" );
	dia.editArguments->setText( MetaDataBase::pixmapArgument( fw, old.serialNumber() ) );
	dia.editArguments->setFocus();
	if ( dia.exec() == QDialog::Accepted ) {
	    QPixmap pix;
	    // we have to force the pixmap to get a new and unique serial number. Unfortunately detatch() doesn't do that
	    pix.convertFromImage( QPixmap::fromMimeSource( "designer_image.png" ).convertToImage() );

	    MetaDataBase::setPixmapArgument( fw, pix.serialNumber(), dia.editArguments->text() );
	    return pix;
	}
    }
#else
    Q_UNUSED( parent );
    Q_UNUSED( fw );
    Q_UNUSED( old );
#endif
    return QPixmap();
}

ImageIconProvider::ImageIconProvider( QWidget *parent, const char *name )
    : QFileIconProvider( parent, name ), imagepm( QPixmap::fromMimeSource( "designer_image.png" ) )
{
    fmts = QImage::inputFormats();
}

ImageIconProvider::~ImageIconProvider()
{
}

const QPixmap * ImageIconProvider::pixmap( const QFileInfo &fi )
{
    QString ext = fi.extension().upper();
    if ( fmts.contains( ext ) ) {
	return &imagepm;
    } else {
	return QFileIconProvider::pixmap( fi );
    }
}
