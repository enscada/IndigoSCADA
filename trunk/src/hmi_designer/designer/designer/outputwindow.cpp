/**********************************************************************
** Copyright (C) 2001-2002 Trolltech AS.  All rights reserved.
**
** This file is part of Qt Designer.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#include "outputwindow.h"
#include "designerappiface.h"
#include "metadatabase.h"
#include "mainwindow.h"

#include <qlistview.h>
#include <qtextedit.h>
#include <qapplication.h>
#include <qheader.h>
#include <stdlib.h>
#include <stdio.h>
#include <qpainter.h>

static QTextEdit *debugoutput = 0;
bool debugToStderr = FALSE;

QtMsgHandler OutputWindow::oldMsgHandler = 0;

OutputWindow::OutputWindow( QWidget *parent )
    : QTabWidget( parent, "output_window" ), debugView( 0 ), errorView( 0 )
{
    setupDebug();
    setupError();
    iface = new DesignerOutputDockImpl( this );
}

OutputWindow::~OutputWindow()
{
    debugoutput = debugView = 0;
    errorView = 0;
    if ( !debugToStderr )
	qInstallMsgHandler( oldMsgHandler );
    delete iface;
}

void OutputWindow::shuttingDown()
{
    if ( !debugToStderr )
	qInstallMsgHandler( oldMsgHandler );
}

void OutputWindow::setupError()
{
    errorView = new QListView( this, "OutputWindow::errorView" );
    errorView->setSorting( -1 );
    connect( errorView, SIGNAL( currentChanged( QListViewItem* ) ),
	     this, SLOT( currentErrorChanged( QListViewItem* ) ) );
    connect( errorView, SIGNAL( clicked( QListViewItem* ) ),
	     this, SLOT( currentErrorChanged( QListViewItem* ) ) );

    if ( MetaDataBase::languages().count() > 1 )
	addTab( errorView, tr( "Warnings/Errors" ) );
    else
	errorView->hide();
    errorView->addColumn( tr( "Type" ) );
    errorView->addColumn( tr( "Message" ) );
    errorView->addColumn( tr( "Line" ) );
    errorView->addColumn( tr( "Location" ) );
    errorView->setResizeMode( QListView::LastColumn );
    errorView->setColumnWidth( 0, errorView->fontMetrics().width( "WARNING1234" ) );
    errorView->setColumnWidth( 1, errorView->fontMetrics().width( "ABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOPQRSTUVWXYZABCDEFGHIJKLMNOP" ) );
    errorView->setColumnWidth( 2, errorView->fontMetrics().width( "9999999" ) );
    errorView->setColumnAlignment( 2, Qt::AlignRight );
    errorView->setAllColumnsShowFocus( TRUE );
}

static void debugMessageOutput( QtMsgType type, const char *msg )
{
    QString s( msg );
    s += "\n";

    if ( type != QtFatalMsg ) {
	if ( debugoutput && debugoutput->isVisible() )
	    debugoutput->append( s );
	else if ( OutputWindow::oldMsgHandler )
	    (*OutputWindow::oldMsgHandler)( type, msg );
	else
	    fputs( s.latin1(), stderr );
    } else {
	fputs( s.latin1(), stderr );
	abort();
    }

    qApp->flush();
}

void OutputWindow::setupDebug()
{
    debugoutput = debugView = new QTextEdit( this, "OutputWindow::debugView" );
    //debugView->setReadOnly( TRUE );
    addTab( debugView, "Debug Output" );

    if ( !debugToStderr )
	oldMsgHandler = qInstallMsgHandler( debugMessageOutput );
}

void OutputWindow::setErrorMessages( const QStringList &errors, const QValueList<uint> &lines,
				     bool clear, const QStringList &locations,
				     const QObjectList &locationObjects )
{
    if ( clear )
	errorView->clear();
    QStringList::ConstIterator mit = errors.begin();
    QValueList<uint>::ConstIterator lit = lines.begin();
    QStringList::ConstIterator it = locations.begin();
    QObjectList objects = (QObjectList)locationObjects;
    QObject *o = objects.first();
    QListViewItem *after = 0;
    for ( ; lit != lines.end() && mit != errors.end(); ++lit, ++mit, ++it, o = objects.next() )
	after = new ErrorItem( errorView, after, *mit, *lit, *it, o );
    setCurrentPage( 1 );
}

DesignerOutputDock *OutputWindow::iFace()
{
    return iface;
}

void OutputWindow::appendDebug( const QString &text )
{
    debugView->append( text + "\n" );
}

void OutputWindow::clearErrorMessages()
{
    errorView->clear();
}

void OutputWindow::clearDebug()
{
    debugView->clear();
}

void OutputWindow::showDebugTab()
{
    showPage( debugView );
}

void OutputWindow::currentErrorChanged( QListViewItem *i )
{
    if ( !i )
	return;
    ErrorItem *ei = (ErrorItem*)i;
    ei->setRead( TRUE );
    MainWindow::self->showSourceLine( ei->location(), ei->line() - 1, MainWindow::Error );
}



ErrorItem::ErrorItem( QListView *parent, QListViewItem *after, const QString &message, int line,
		      const QString &locationString, QObject *locationObject )
    : QListViewItem( parent, after )
{
    setMultiLinesEnabled( TRUE );
    QString m( message );
    type = m.startsWith( "Warning: " ) ? Warning : Error;
    m = m.mid( m.find( ':' ) + 1 );
    setText( 0, type == Error ? "Error" : "Warning" );
    setText( 1, m );
    setText( 2, QString::number( line ) );
    setText( 3, locationString );
    object = locationObject;
    read = !after;
    if ( !after ) {
	parent->setSelected( this, TRUE );
	parent->setCurrentItem( this );
    }
}

void ErrorItem::paintCell( QPainter *p, const QColorGroup & cg,
			   int column, int width, int alignment )
{
    QColorGroup g( cg );
    g.setColor( QColorGroup::Text, type == Error ? Qt::red : Qt::darkYellow );
    if ( !read ) {
	QFont f( p->font() );
	f.setBold( TRUE );
	p->setFont( f );
    }
    QListViewItem::paintCell( p, g, column, width, alignment );
}
