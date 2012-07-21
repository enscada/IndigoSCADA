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

#ifndef CONNECTIONTABLE_H
#define CONNECTIONTABLE_H

#include <qtable.h>

class ConnectionTable : public QTable
{
    Q_OBJECT

public:
    ConnectionTable( QWidget *parent, const char *name );

    void sortColumn( int col, bool ascending, bool wholeRows );

signals:
    void resorted();

};

#endif
