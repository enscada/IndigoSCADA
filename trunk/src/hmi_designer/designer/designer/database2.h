/**********************************************************************
** Copyright (C) 2000 Trolltech AS.  All rights reserved.
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

#ifndef DATABASE2_H
#define DATABASE2_H

#include "qfeatures.h"

#ifndef QT_NO_SQL
#include <qdataview.h>
#include <qdatabrowser.h>
#include <qsqlcursor.h>
#include <qstring.h>

class QSqlDatabase;
class QSqlForm;

class DatabaseSupport2
{
public:
    DatabaseSupport2();
    virtual ~DatabaseSupport2() {}

    void initPreview( const QString &connection, const QString &table, QObject *o,
		      const QMap<QString, QString> &databaseControls );

protected:
    QSqlDatabase* con;
    QSqlForm* frm;
    QString tbl;
    QMap<QString, QString> dbControls;
    QObject *parent;

};

class QDesignerDataBrowser2 : public QDataBrowser, public DatabaseSupport2
{
    Q_OBJECT

public:
    QDesignerDataBrowser2( QWidget *parent, const char *name );

protected:
    bool event( QEvent* e );
};

class QDesignerDataView2 : public QDataView, public DatabaseSupport2
{
    Q_OBJECT

public:
    QDesignerDataView2( QWidget *parent, const char *name );

protected:
    bool event( QEvent* e );

};

#define DatabaseSupport DatabaseSupport2
#define QDesignerDataBrowser QDesignerDataBrowser2
#define QDesignerDataView QDesignerDataView2

#endif

#endif
