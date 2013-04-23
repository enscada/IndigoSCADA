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

#ifndef OUTPUTWINDOW_H
#define OUTPUTWINDOW_H

#include <qtabwidget.h>
#include <qstringlist.h>
#include <qvaluelist.h>
#include <qlistview.h>

struct DesignerOutputDock;
class QTextEdit;
class QListView;

class ErrorItem : public QListViewItem
{
public:
    enum Type { Error, Warning };

    ErrorItem( QListView *parent, QListViewItem *after, const QString &message, int line,
	       const QString &locationString, QObject *locationObject );

    void paintCell( QPainter *, const QColorGroup & cg,
		    int column, int width, int alignment );

    void setRead( bool b ) { read = b; repaint(); }

    QObject *location() const { return object; }
    int line() const { return text( 2 ).toInt(); }

private:
    QObject *object;
    Type type;
    bool read;

};

class OutputWindow : public QTabWidget
{
    Q_OBJECT

public:
    OutputWindow( QWidget *parent );
    ~OutputWindow();

    void setErrorMessages( const QStringList &errors, const QValueList<uint> &lines,
			   bool clear, const QStringList &locations,
			   const QObjectList &locationObjects );
    void appendDebug( const QString& );
    void clearErrorMessages();
    void clearDebug();
    void showDebugTab();

    DesignerOutputDock *iFace();

    void shuttingDown();

    static QtMsgHandler oldMsgHandler;

private slots:
    void currentErrorChanged( QListViewItem *i );

private:
    void setupError();
    void setupDebug();

    QTextEdit *debugView;
    QListView *errorView;

    DesignerOutputDock *iface;

};

#endif
