/**********************************************************************
** Copyright (C) 2000-2002 Trolltech AS.  All rights reserved.
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

#ifndef DBCONNECTIONIMPL_H
#define DBCONNECTIONIMPL_H

#include "dbconnectioneditor.h"

class DatabaseConnection;
class DatabaseConnectionWidget;
class QGridLayout;

class DatabaseConnectionEditor : public DatabaseConnectionEditorBase
{
    Q_OBJECT

public:
    DatabaseConnectionEditor( DatabaseConnection* connection, QWidget* parent = 0,
			       const char* name = 0, bool modal = FALSE, WFlags fl = 0 );
    ~DatabaseConnectionEditor();

public slots:
    void accept();
private:
    void init();

private:
    DatabaseConnection *conn;
    QGridLayout* lay;
    DatabaseConnectionWidget* connectionWidget;

};

#endif // DBCONNECTIONIMPL_H
