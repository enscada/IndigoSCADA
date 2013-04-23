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

#ifndef DESIGNERAPP_H
#define DESIGNERAPP_H

class QSplashScreen;

#include <qapplication.h>

class DesignerApplication : public QApplication
{
public:
    const char *className() const { return "DesignerApplication"; }

    DesignerApplication( int &argc, char **argv );

    QSplashScreen *showSplash();
    static void closeSplash();

    static QString settingsKey();
    static QString oldSettingsKey();
    static void setSettingsKey( const QString &key );

protected:
    QDateTime lastMod;

#if defined(Q_WS_WIN)
    bool winEventFilter( MSG *msg );
    uint DESIGNER_OPENFILE;
#endif

};

#endif
