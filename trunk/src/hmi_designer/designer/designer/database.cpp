/**********************************************************************
** Copyright (C) 2005-2007 Trolltech ASA.  All rights reserved.
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

#if defined(DESIGNER)
#include "database.h"
#else
#include "database2.h"
#endif

#ifndef QT_NO_SQL

#if defined(DESIGNER)
#include "formwindow.h"
#include "mainwindow.h"
#endif

#include <qsqldatabase.h>
#include <qsqlform.h>
#include <qsqlcursor.h>
#include <qsqlrecord.h>

DatabaseSupport::DatabaseSupport()
{
    con = 0;
    frm = 0;
    parent = 0;
}

void DatabaseSupport::initPreview( const QString &connection, const QString &table, QObject *o,
				   const QMap<QString, QString> &databaseControls )
{
    tbl = table;
    dbControls = databaseControls;
    parent = o;

    if ( connection != "(default)" )
	con = QSqlDatabase::database( connection );
    else
	con = QSqlDatabase::database();
    frm = new QSqlForm( o, table );
    for ( QMap<QString, QString>::Iterator it = dbControls.begin(); it != dbControls.end(); ++it ) {
	QObject *chld = parent->child( it.key(), "QWidget" );
	if ( !chld )
	    continue;
	frm->insert( (QWidget*)chld, *it );
    }
}

QDesignerDataBrowser::QDesignerDataBrowser( QWidget *parent, const char *name )
    : QDataBrowser( parent, name )
{
}

bool QDesignerDataBrowser::event( QEvent* e )
{
    bool b = QDataBrowser::event( e );
#if defined(DESIGNER)
    if ( MainWindow::self->isPreviewing() ) {
#endif
	if ( e->type() == QEvent::Show ) {
	    if ( con ) {
		QSqlCursor* cursor = new QSqlCursor( tbl, TRUE, con );
		setSqlCursor( cursor, TRUE );
		setForm( frm );
		refresh();
		first();
	    }
	    return TRUE;
	}
#if defined(DESIGNER)
    }
#endif
    return b;
}

QDesignerDataView::QDesignerDataView( QWidget *parent, const char *name )
    : QDataView( parent, name )
{
}

bool QDesignerDataView::event( QEvent* e )
{
    bool b = QDataView::event( e );
#if defined(DESIGNER)
    if ( MainWindow::self->isPreviewing() ) {
#endif
	if ( e->type() == QEvent::Show ) {
	    setForm( frm );
	    readFields();
	    return TRUE;
	}
#if defined(DESIGNER)
    }
#endif
    return b;
}


#endif
