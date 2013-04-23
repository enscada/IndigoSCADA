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

#ifndef METADATABASE_H
#define METADATABASE_H

#include <qvariant.h>
#include <qstring.h>
#include <qstringlist.h>
#include <qmap.h>
#include <qptrlist.h>
#include <qsizepolicy.h>
#include <qsize.h>
#include <qwidgetlist.h>
#include <qcursor.h>

#include "pixmapchooser.h"
#include "../interfaces/languageinterface.h"

class QObject;
class QPixmap;
struct LanguageInterface;

class MetaDataBase
{
public:
    struct Connection
    {
	QObject *sender, *receiver;
	QCString signal, slot;
	bool operator==( const Connection &c ) const {
	    return sender == c.sender && receiver == c.receiver &&
		   signal == c.signal && slot == c.slot ;
	}
    };

    struct Function
    {
	QString returnType;
	QCString function;
	QString specifier;
	QString access;
	QString type;
	QString language;
	bool operator==( const Function &f ) const {
	    return ( returnType == f.returnType &&
		     function == f.function &&
		     specifier == f.specifier &&
		     access == f.access &&
		     type == f.type &&
		     language == f.language
		     );
	}
    };

    struct Property
    {
	QCString property;
	QString type;
	bool operator==( const Property &p ) const {
	    return property == p.property &&
		 type == p.type;
	}
    };

    struct CustomWidget
    {
	CustomWidget();
	CustomWidget( const CustomWidget &w );
	~CustomWidget() { delete pixmap; } // inlined to work around 2.7.2.3 bug
	bool operator==( const CustomWidget &w ) const;
	CustomWidget &operator=( const CustomWidget &w );

	bool hasSignal( const QCString &signal ) const;
	bool hasSlot( const QCString &slot ) const;
	bool hasProperty( const QCString &prop ) const;

	enum IncludePolicy { Global, Local };
	QString className;
	QString includeFile;
	IncludePolicy includePolicy;
	QSize sizeHint;
	QSizePolicy sizePolicy;
	QPixmap *pixmap;
	QValueList<QCString> lstSignals;
	QValueList<Function> lstSlots;
	QValueList<Property> lstProperties;
	int id;
	bool isContainer;
    };

    struct Include
    {
	Include() : header(), location(), implDecl( "in implementation" ) {}
	QString header;
	QString location;
	bool operator==( const Include &i ) const {
	    return header == i.header && location == i.location;
	}
	QString implDecl;
    };

    struct Variable
    {
	QString varName;
	QString varAccess;
	bool operator==( const Variable &v ) const {
	    return varName == v.varName &&
		   varAccess == v.varAccess;
	}
    };

    struct MetaInfo
    {
	MetaInfo() : classNameChanged( FALSE ) { }
	QString className;
	bool classNameChanged;
	QString comment;
	QString author;
    };

    MetaDataBase();
    static void clearDataBase();

    static void addEntry( QObject *o );
    static void removeEntry( QObject *o );
    static void setPropertyChanged( QObject *o, const QString &property, bool changed );
    static bool isPropertyChanged( QObject *o, const QString &property );
    static void setPropertyComment( QObject *o, const QString &property, const QString &comment );
    static QString propertyComment( QObject *o, const QString &property );
    static QStringList changedProperties( QObject *o );

    static void setFakeProperty( QObject *o, const QString &property, const QVariant& value );
    static QVariant fakeProperty( QObject * o, const QString &property );
    static QMap<QString,QVariant>* fakeProperties( QObject* o );

    static void setSpacing( QObject *o, int spacing );
    static int spacing( QObject *o );
    static void setMargin( QObject *o, int margin );
    static int margin( QObject *o );

    static void setResizeMode( QObject *o, const QString &mode );
    static QString resizeMode( QObject *o );

    static void addConnection( QObject *o, QObject *sender, const QCString &signal,
			       QObject *receiver, const QCString &slot, bool addCode = TRUE );
    static void removeConnection( QObject *o, QObject *sender, const QCString &signal,
				  QObject *receiver, const QCString &slot );
    static bool hasConnection( QObject *o, QObject *sender, const QCString &signal,
			       QObject *receiver, const QCString &slot );
    static void setupConnections( QObject *o, const QValueList<LanguageInterface::Connection> &conns );
    static QValueList<Connection> connections( QObject *o );
    static QValueList<Connection> connections( QObject *o, QObject *sender, QObject *receiver );
    static QValueList<Connection> connections( QObject *o, QObject *object );
    static void doConnections( QObject *o );

    static void addFunction( QObject *o, const QCString &function, const QString &specifier,
			     const QString &access, const QString &type, const QString &language,
			     const QString &returnType );
    static void removeFunction( QObject *o, const QCString &function, const QString &specifier,
				const QString &access, const QString &type, const QString &language,
				const QString &returnType );
    static void removeFunction( QObject *o, const QString &function );
    static QValueList<Function> functionList( QObject *o, bool onlyFunctions = FALSE );
    static QValueList<Function> slotList( QObject *o );
    static bool isSlotUsed( QObject *o, const QCString &slot );
    static bool hasFunction( QObject *o, const QCString &function, bool onlyCustom = FALSE );
    static bool hasSlot( QObject *o, const QCString &slot, bool onlyCustom = FALSE );
    static void changeFunction( QObject *o, const QString &function, const QString &newName,
				const QString &returnType );
    static void changeFunctionAttributes( QObject *o, const QString &oldName, const QString &newName,
				      const QString &specifier, const QString &access,
				      const QString &type, const QString &language,
				      const QString &returnType );
    static QString languageOfFunction( QObject *o, const QCString &function );
    static void setFunctionList( QObject *o, const QValueList<Function> &functionList );


    static bool addCustomWidget( CustomWidget *w );
    static void removeCustomWidget( CustomWidget *w );
    static QPtrList<CustomWidget> *customWidgets();
    static CustomWidget *customWidget( int id );
    static bool isWidgetNameUsed( CustomWidget *w );
    static bool hasCustomWidget( const QString &className );

    static void setTabOrder( QWidget *w, const QWidgetList &order );
    static QWidgetList tabOrder( QWidget *w );

    static void setIncludes( QObject *o, const QValueList<Include> &incs );
    static QValueList<Include> includes( QObject *o );

    static void setForwards( QObject *o, const QStringList &fwds );
    static QStringList forwards( QObject *o );

    static void setVariables( QObject *o, const QValueList<Variable> &vars );
    static void addVariable( QObject *o, const QString &name, const QString &access );
    static void removeVariable( QObject *o, const QString &name );
    static QValueList<Variable> variables( QObject *o );
    static bool hasVariable( QObject *o, const QString &name );
    static QString extractVariableName( const QString &name );

    static void setSignalList( QObject *o, const QStringList &sigs );
    static QStringList signalList( QObject *o );

    static void setMetaInfo( QObject *o, MetaInfo mi );
    static MetaInfo metaInfo( QObject *o );

    static void setCursor( QWidget *w, const QCursor &c );
    static QCursor cursor( QWidget *w );

    static void setPixmapArgument( QObject *o, int pixmap, const QString &arg );
    static QString pixmapArgument( QObject *o, int pixmap );
    static void clearPixmapArguments( QObject *o );

    static void setPixmapKey( QObject *o, int pixmap, const QString &arg );
    static QString pixmapKey( QObject *o, int pixmap );
    static void clearPixmapKeys( QObject *o );

    static void setColumnFields( QObject *o, const QMap<QString, QString> &columnFields );
    static QMap<QString, QString> columnFields( QObject *o );

    static void setEditor( const QStringList &langs );
    static bool hasEditor( const QString &lang );

    static void setupInterfaceManagers( const QString &plugDir );
    static QStringList languages();

    static LanguageInterface *languageInterface( const QString &lang );

    static QString normalizeFunction( const QString &f );

    static void clear( QObject *o );

    static void setBreakPoints( QObject *o, const QValueList<uint> &l );
    static void setBreakPointCondition( QObject *o, int line, const QString &condition );
    static QValueList<uint> breakPoints( QObject *o );
    static QString breakPointCondition( QObject *o, int line );

    static void setExportMacro( QObject *o, const QString &macro );
    static QString exportMacro( QObject *o );

    static bool hasObject( QObject *o );

};

#endif
