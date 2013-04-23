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

#ifndef WIDGETFACTORY_H
#define WIDGETFACTORY_H

#include <qvariant.h>
#include <qiconset.h>
#include <qstring.h>
#include <qintdict.h>
#include <qtabwidget.h>
#include <qpixmap.h>
#include <qsize.h>
#include <qpainter.h>
#include <qevent.h>
#include <qobjectlist.h>
#include <qlabel.h>
#include <qwizard.h>
#include <qptrdict.h>
#include <qpushbutton.h>
#include <qtoolbutton.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qwidgetstack.h>
#include <qguardedptr.h>
#include <qtoolbox.h>

#include "metadatabase.h"
#include "qwidgetfactory.h"

class QWidget;
class QLayout;
class FormWindow;

class CustomWidgetFactory : public QWidgetFactory
{
public:
    CustomWidgetFactory();
    QWidget *createWidget( const QString &className, QWidget *parent, const char *name ) const;

};

class WidgetFactory : public Qt
{
    friend class CustomWidgetFactory;

public:
    enum LayoutType {
	HBox,
	VBox,
	Grid,
	NoLayout
    };

    static QWidget *create( int id, QWidget *parent, const char *name = 0, bool init = TRUE,
			    const QRect *rect = 0, Qt::Orientation orient = Qt::Horizontal );
    static QLayout *createLayout( QWidget *widget, QLayout* layout, LayoutType type );
    static void deleteLayout( QWidget *widget );

    static LayoutType layoutType( QWidget *w );
    static LayoutType layoutType( QWidget *w, QLayout *&layout );
    static LayoutType layoutType( QLayout *layout );
    static QWidget *layoutParent( QLayout *layout );

    static QWidget* containerOfWidget( QWidget *w );
    static QWidget* widgetOfContainer( QWidget *w );

    static bool isPassiveInteractor( QObject* o );
    static const char* classNameOf( QObject* o );

    static void initChangedProperties( QObject *o );

    static bool hasSpecialEditor( int id, QObject *editorWidget );
    static bool hasItems( int id, QObject *editorWidget );
    static void editWidget( int id, QWidget *parent, QWidget *editWidget, FormWindow *fw );

    static bool canResetProperty( QObject *w, const QString &propName );
    static bool resetProperty( QObject *w, const QString &propName );
    static QVariant defaultValue( QObject *w, const QString &propName );
    static QString defaultCurrentItem( QObject *w, const QString &propName );

    static QVariant property( QObject *w, const char *name );
    static void saveDefaultProperties( QObject *w, int id );
    static void saveChangedProperties( QObject *w, int id );

    static QString defaultSignal( QObject *w );

private:
    static QWidget *createWidget( const QString &className, QWidget *parent, const char *name, bool init,
				  const QRect *r = 0, Qt::Orientation orient = Qt::Horizontal );
    static QWidget *createCustomWidget( QWidget *parent, const char *name, MetaDataBase::CustomWidget *w );

    static QGuardedPtr<QObject> *lastPassiveInteractor;
    static bool lastWasAPassiveInteractor;
};


class QDesignerTabWidget : public QTabWidget
{
    Q_OBJECT
    Q_PROPERTY( int currentPage READ currentPage WRITE setCurrentPage STORED false DESIGNABLE true )
    Q_PROPERTY( QString pageTitle READ pageTitle WRITE setPageTitle STORED false DESIGNABLE true )
    Q_PROPERTY( QCString pageName READ pageName WRITE setPageName STORED false DESIGNABLE true )
public:
    QDesignerTabWidget( QWidget *parent, const char *name );

    int currentPage() const;
    void setCurrentPage( int i );
    QString pageTitle() const;
    void setPageTitle( const QString& title );
    QCString pageName() const;
    void setPageName( const QCString& name );

    int count() const;
    QTabBar *tabBar() const { return QTabWidget::tabBar(); }

    bool eventFilter( QObject*, QEvent* );

private:
    QPoint pressPoint;
    QWidget *dropIndicator;
    QWidget *dragPage;
    QString dragLabel;
     bool mousePressed;
};

class QDesignerWidgetStack : public QWidgetStack
{
    Q_OBJECT
    Q_PROPERTY( int currentPage READ currentPage WRITE setCurrentPage STORED false DESIGNABLE true )
    Q_PROPERTY( QCString pageName READ pageName WRITE setPageName STORED false DESIGNABLE true )
public:
    QDesignerWidgetStack( QWidget *parent, const char *name );

    int currentPage() const;
    void setCurrentPage( int i );
    QCString pageName() const;
    void setPageName( const QCString& name );

    int count() const;
    QWidget* page( int i ) const;

    int insertPage( QWidget *p, int i = -1 );
    int removePage( QWidget *p );

public slots:
    void updateButtons();

protected:
    void resizeEvent( QResizeEvent *e ) {
	QWidgetStack::resizeEvent( e );
	updateButtons();
    }

    void showEvent( QShowEvent *e ) {
	QWidgetStack::showEvent( e );
	updateButtons();
    }

private slots:
    void prevPage();
    void nextPage();

private:
    QPtrList<QWidget> pages;
    QToolButton *prev, *next;

};

class QDesignerWizard : public QWizard
{
    Q_OBJECT
    Q_PROPERTY( int currentPage READ currentPageNum WRITE setCurrentPage STORED false DESIGNABLE true )
    Q_PROPERTY( QString pageTitle READ pageTitle WRITE setPageTitle STORED false DESIGNABLE true )
    Q_PROPERTY( QCString pageName READ pageName WRITE setPageName STORED false DESIGNABLE true )
    Q_OVERRIDE( bool modal READ isModal WRITE setModal )

public:
    QDesignerWizard( QWidget *parent, const char *name ) 
        : QWizard( parent, name ), modal(FALSE) {}

    int currentPageNum() const;
    void setCurrentPage( int i );
    QString pageTitle() const;
    void setPageTitle( const QString& title );
    QCString pageName() const;
    void setPageName( const QCString& name );
    int pageNum( QWidget *page );
    void addPage( QWidget *p, const QString & );
    void removePage( QWidget *p );
    void insertPage( QWidget *p, const QString &t, int index );
    bool isPageRemoved( QWidget *p ) { return (removedPages.find( p ) != 0); }

    bool isModal() const { return modal; }
    void setModal(bool b) { modal = b; }

    void reject() {}

private:
    struct Page
    {
	Page( QWidget *a, const QString &b ) : p( a ), t( b ) {}
	Page() : p( 0 ), t( QString::null ) {}
	QWidget *p;
	QString t;
    };
    QPtrDict<QWidget> removedPages;
    bool modal;

};

class QLayoutWidget : public QWidget
{
    Q_OBJECT

public:
    QLayoutWidget( QWidget *parent, const char *name ) : QWidget( parent, name ), sp( QWidget::sizePolicy() ) {}

    QSizePolicy sizePolicy() const;
    void updateSizePolicy();

protected:
    void paintEvent( QPaintEvent * );
    bool event( QEvent * );
    QSizePolicy sp;

};


class CustomWidget : public QWidget
{
    Q_OBJECT

public:
    CustomWidget( QWidget *parent, const char *name, MetaDataBase::CustomWidget *cw )
	: QWidget( parent, name ), cusw( cw ) {
	    alwaysExpand = parentWidget() && parentWidget()->inherits( "FormWindow" );
	    setSizePolicy( cw->sizePolicy );
	    if ( !alwaysExpand )
		setBackgroundMode( PaletteDark );
    }

    QSize sizeHint() const {
	QSize sh = cusw->sizeHint;
	if ( sh.isValid() )
	    return sh;
	return QWidget::sizeHint();
    }

    QString realClassName() { return cusw->className; }
    MetaDataBase::CustomWidget *customWidget() const { return cusw; }

protected:
    void paintEvent( QPaintEvent *e );

    MetaDataBase::CustomWidget *cusw;
    bool alwaysExpand;

};


class Line : public QFrame
{
    Q_OBJECT

    Q_PROPERTY( Orientation orientation READ orientation WRITE setOrientation )
    Q_OVERRIDE( int frameWidth DESIGNABLE false )
    Q_OVERRIDE( Shape frameShape DESIGNABLE false )
    Q_OVERRIDE( QRect frameRect DESIGNABLE false )
    Q_OVERRIDE( QRect contentsRect DESIGNABLE false )
public:
    Line( QWidget *parent, const char *name )
	: QFrame( parent, name, WMouseNoMask ) {
	    setFrameStyle( HLine | Sunken );
    }

    void setOrientation( Orientation orient ) {
	if ( orient == Horizontal )
	    setFrameShape( HLine );
	else
	    setFrameShape( VLine );
    }
    Orientation orientation() const {
	return frameShape() == HLine ? Horizontal : Vertical;
    }
};

class QDesignerLabel : public QLabel
{
    Q_OBJECT

    Q_PROPERTY( QCString buddy READ buddyWidget WRITE setBuddyWidget )

public:
    QDesignerLabel( QWidget *parent = 0, const char *name = 0 )
	: QLabel( parent, name ) { myBuddy = 0; }

    void setBuddyWidget( const QCString &b ) {
	myBuddy = b;
	updateBuddy();
    }
    QCString buddyWidget() const {
	return myBuddy;
    };

protected:
    void showEvent( QShowEvent *e ) {
	QLabel::showEvent( e );
	updateBuddy();
    }


private:
    void updateBuddy();

    QCString myBuddy;

};

class QDesignerWidget : public QWidget
{
    Q_OBJECT

public:
    QDesignerWidget( FormWindow *fw, QWidget *parent, const char *name )
	: QWidget( parent, name, WResizeNoErase ), formwindow( fw ) {
	    need_frame = parent && parent->inherits("QDesignerWidgetStack" );
    }

protected:
    void resizeEvent( QResizeEvent* e);
    void paintEvent( QPaintEvent *e );

private:
    FormWindow *formwindow;
    uint need_frame : 1;

};

class QDesignerDialog : public QDialog
{
    Q_OBJECT
    Q_OVERRIDE( bool modal READ isModal WRITE setModal )

public:
    QDesignerDialog( FormWindow *fw, QWidget *parent, const char *name )
	: QDialog( parent, name, FALSE, WResizeNoErase ), formwindow( fw ), modal(FALSE) {}

    bool isModal() const { return modal; }
    void setModal(bool b) { modal = b; }

protected:
    void paintEvent( QPaintEvent *e );

private:
    FormWindow *formwindow;
    bool modal;

};

class QDesignerToolButton : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY( int buttonGroupId READ buttonGroupId WRITE setButtonGroupId )

public:
    QDesignerToolButton( QWidget *parent, const char *name )
	: QToolButton( parent, name ) {}

    bool isInButtonGroup() const {
	return parentWidget() && parentWidget()->inherits( "QButtonGroup" );
    }
    int buttonGroupId() const {
	return parentWidget() && parentWidget()->inherits( "QButtonGroup" ) ? ( (QButtonGroup*)parentWidget() )->id( (QButton*)this ) : -1;
    }
    void setButtonGroupId( int id ) {
	if ( parentWidget() && parentWidget()->inherits( "QButtonGroup" ) ) {
	    ( (QButtonGroup*)parentWidget() )->remove( this );
	    ( (QButtonGroup*)parentWidget() )->insert( this, id );
	}
    }
};

class QDesignerRadioButton : public QRadioButton
{
    Q_OBJECT
    Q_PROPERTY( int buttonGroupId READ buttonGroupId WRITE setButtonGroupId )

public:
    QDesignerRadioButton( QWidget *parent, const char *name )
	: QRadioButton( parent, name ) {}

    bool isInButtonGroup() const {
	return parentWidget() && parentWidget()->inherits( "QButtonGroup" );
    }
    int buttonGroupId() const {
	return parentWidget() && parentWidget()->inherits( "QButtonGroup" ) ? ( (QButtonGroup*)parentWidget() )->id( (QButton*)this ) : -1;
    }
    void setButtonGroupId( int id ) {
	if ( parentWidget() && parentWidget()->inherits( "QButtonGroup" ) ) {
	    ( (QButtonGroup*)parentWidget() )->remove( this );
	    ( (QButtonGroup*)parentWidget() )->insert( this, id );
	}
    }

    void setFocusPolicy( FocusPolicy policy );
};

class QDesignerPushButton : public QPushButton
{
    Q_OBJECT
    Q_PROPERTY( int buttonGroupId READ buttonGroupId WRITE setButtonGroupId )

public:
    QDesignerPushButton( QWidget *parent, const char *name )
	: QPushButton( parent, name ) {}

    bool isInButtonGroup() const {
	return parentWidget() && parentWidget()->inherits( "QButtonGroup" );
    }
    int buttonGroupId() const {
	return parentWidget() && parentWidget()->inherits( "QButtonGroup" ) ? ( (QButtonGroup*)parentWidget() )->id( (QButton*)this ) : -1;
    }
    void setButtonGroupId( int id ) {
	if ( parentWidget() && parentWidget()->inherits( "QButtonGroup" ) ) {
	    ( (QButtonGroup*)parentWidget() )->remove( this );
	    ( (QButtonGroup*)parentWidget() )->insert( this, id );
	}
    }

};

class QDesignerCheckBox : public QCheckBox
{
    Q_OBJECT
    Q_PROPERTY( int buttonGroupId READ buttonGroupId WRITE setButtonGroupId )

public:
    QDesignerCheckBox( QWidget *parent, const char *name )
	: QCheckBox( parent, name ) {}

    bool isInButtonGroup() const {
	return parentWidget() && parentWidget()->inherits( "QButtonGroup" );
    }
    int buttonGroupId() const {
	return parentWidget() && parentWidget()->inherits( "QButtonGroup" ) ? ( (QButtonGroup*)parentWidget() )->id( (QButton*)this ) : -1;
    }
    void setButtonGroupId( int id ) {
	if ( parentWidget() && parentWidget()->inherits( "QButtonGroup" ) ) {
	    ( (QButtonGroup*)parentWidget() )->remove( this );
	    ( (QButtonGroup*)parentWidget() )->insert( this, id );
	}
    }

};

class QDesignerToolBox : public QToolBox
{
    Q_OBJECT
    Q_PROPERTY( QString itemLabel READ itemLabel WRITE setItemLabel STORED false DESIGNABLE true )
    Q_PROPERTY( QCString itemName READ itemName WRITE setItemName STORED false DESIGNABLE true )
    Q_PROPERTY( BackgroundMode itemBackgroundMode READ itemBackgroundMode WRITE setItemBackgroundMode STORED false DESIGNABLE true )

public:
    QDesignerToolBox( QWidget *parent, const char *name );

    QString itemLabel() const;
    void setItemLabel( const QString &l );
    QCString itemName() const;
    void setItemName( const QCString &n );
    BackgroundMode itemBackgroundMode() const;
    void setItemBackgroundMode( BackgroundMode );

protected:
    void itemInserted( int index );
};

#endif
