/**********************************************************************
--- Qt Architect generated file ---
File: TagCfg.cpp
Last generated: Thu Apr 13 16:29:45 2000
*********************************************************************/
#include "TagCfg.h"
#define Inherited TagCfgData
#include "main.h"
#include "dbase.h"
#include "common.h"
#include "driver.h"
#include "sptypes.h"
#include "valedit.h"

#include "IndentedTrace.h"

TagCfg::TagCfg
(QWidget * parent, 
const QString &spname, 
const QString &type, 
const QString &unittype,
const QString &receipename,
const QString &unitname,
const char *name)
:Inherited (parent, name), 
pTimer (new QTimer (this)),
SPName (spname),
Type(type),
UnitType(unittype),
ReceipeName(receipename)
{
	setCaption (tr ("Alarm Limit Configuration Receipe") + "[" + ReceipeName + "]");
	connect (GetConfigureDb (),
	SIGNAL (TransactionDone (QObject *, const QString &, int, QObject*)), this,
	SLOT (QueryResponse (QObject *, const QString &, int, QObject*)));	// connect to the databas
	Driver *p = FindDriver(UnitType); // find theDLL
	if(p)
	{
		QStringList list;
		p->GetTagList(Type,list,unitname,spname);
		Name->clear(); // put them in the list box
		QStringList::Iterator it = list.begin();
		for(;it != list.end();++it)
		{
			Name->insertItem((*it));
		};
	};
	SelChanged(0);
	connect (pTimer, SIGNAL (timeout ()), this, SLOT (DoSelChange ()));	// wire up the item selection timer
	//
	//set the tab order
	// 
	QWidget *pL[] = {
		Name, UpperAlarm, UAEnabled, UpperWarning, UWEnabled, LowerWarning,
		LWEnabled, LowerAlarm, LAEnabled, 0
	};
	SetTabOrder (this, pL);
	//
	// set the validators on the limit fields
	UpperAlarm->setValidator(new QDoubleValidator(-100000.0,100000.0,3,UpperAlarm));	
	UpperWarning->setValidator(new QDoubleValidator(-100000.0,100000.0,3,UpperWarning));	
	LowerWarning->setValidator(new QDoubleValidator(-100000.0,100000.0,3,LowerWarning));	
	LowerAlarm->setValidator(new QDoubleValidator(-100000.0,100000.0,3,LowerAlarm));	
	//
	//
}
TagCfg::~TagCfg ()
{
}
void TagCfg::Help ()
{
	QSHelp("ConfigureAlarmLimits");
}
void TagCfg::Apply ()
{
	//
	// check the alarm limits are sensible
	//
	{
		int n = 0;
		double v[4];
		if(UAEnabled->isChecked())
		{
			v[n++] = UpperAlarm->text().toDouble();
		};
		if(UWEnabled->isChecked())
		{
			v[n++] = UpperWarning->text().toDouble();
		};
		if(LWEnabled->isChecked())
		{
			v[n++] = LowerWarning->text().toDouble();
		};
		if(LAEnabled->isChecked())
		{
			v[n++] = LowerAlarm->text().toDouble();
		};
		if(n > 1)
		{
			for(int i = 1; i < n; i++)
			{
				if(v[i-1] < v[i])
				{
					// validation failure
					MSG(tr("Limit Validation"),tr("Error in Alarm Limits"));                    
					return;
				};
			};
		};
	};
	//
	Build ();
	//
	QString cmd =
	GetConfigureDb ()->Update ("TAGS","NAME='" + SPName +
	"' and TAG='"+Name->currentText () +
	"' and RECEIPE",ReceipeName);	// generate the update record
	//
	GetConfigureDb ()->DoExec (this,cmd,tApply);	// lock the db and action the request
	ButtonState (false);
	DOAUDIT(tr("Apply:") + SPName + ":" + Name->currentText());
}
void TagCfg::SelChanged (int)
{
	pTimer->stop ();		// cancel any timer action  
	if (Name->count ())
	{
		pTimer->start (100, TRUE);	// after inactivity fetch the record - we want to avoid too much activity
	};
}
void TagCfg::DoSelChange ()
{
	IT_IT("TagCfg::DoSelChange");	
	//
	QString cmd =
	"select * from TAGS where NAME='"+ SPName +
	"'and TAG ='" + Name->currentText() + 
	"' and (RECEIPE='(default)' or RECEIPE='"+ReceipeName+"') order by RECEIPE desc;";
	//
	GetConfigureDb ()->DoExec (this,cmd,tItem);
	ButtonState (false);
};
void TagCfg::Build () // build the update record - the insert needs the name record addedtivityled int2);, tion text,repprint int2);ate text);
{
	GetConfigureDb ()->ClearRecord ();	// clear the record
	GetConfigureDb ()->AddToRecord ("UPPERALARM", UpperAlarm->text (), false);
	GetConfigureDb ()->AddToRecord ("UPPERWARN", UpperWarning->text (),false);
	GetConfigureDb ()->AddToRecord ("LOWERALARM", LowerAlarm->text (), false);
	GetConfigureDb ()->AddToRecord ("LOWERWARN", LowerWarning->text (),false);
	//
	GetConfigureDb ()->AddBool("LAENABLE",LAEnabled->isChecked ());
	GetConfigureDb ()->AddBool("LWENABLE",LWEnabled->isChecked ());
	GetConfigureDb ()->AddBool("UAENABLE",UAEnabled->isChecked ());
	GetConfigureDb ()->AddBool("UWENABLE",UWEnabled->isChecked ());
	// 
};
void TagCfg::QueryResponse (QObject *p, const QString &c, int State, QObject*caller)
{
	if(p != this) return;
	switch (State)
	{
//		case tList:
//		{
//			// fill the name list box
//			GetConfigureDb ()->FillComboBox (Name, "TAG");
//			GetConfigureDb ()->DoneExec (this);
//			Name->setCurrentItem (0);
//			SelChanged (0);
//			Name->setFocus ();
//			ButtonState (true);
//		};
//		break;
		case tItem:
		{
			// 
			// fill the fields     
			// 
			if(GetConfigureDb()->GetNumberResults() > 0)
			{
				//
				// if no previous defintion exists, other than default 
				// then we clone the default 
				// 
				UpperAlarm->setText (GetConfigureDb ()->GetString ("UPPERALARM"));
				UpperWarning->setText (GetConfigureDb ()->GetString ("UPPERWARN"));
				LowerWarning->setText (GetConfigureDb ()->GetString ("LOWERWARN"));
				LowerAlarm->setText (GetConfigureDb ()->GetString ("LOWERALARM"));
				// 
				//uaenable int2, uwenable int2, lwenable int2, laenable int2
				//
				LAEnabled->setChecked (GetConfigureDb ()->GetBool("LAENABLE"));
				LWEnabled->setChecked (GetConfigureDb ()->GetBool("LWENABLE"));
				UWEnabled->setChecked (GetConfigureDb ()->GetBool("UWENABLE"));
				UAEnabled->setChecked (GetConfigureDb ()->GetBool("UAENABLE"));
				//			
				if(ReceipeName != "(default)")
				{
					if(GetConfigureDb()->GetString("RECEIPE") == "(default)") // was it the default receipe that was returned
					{
						Build();
						GetConfigureDb ()->AddToRecord ("NAME",SPName);
						GetConfigureDb ()->AddToRecord ("TAG",Name->currentText());
						GetConfigureDb ()->AddToRecord ("RECEIPE",ReceipeName);
						GetConfigureDb()->DoExec(0,GetConfigureDb()->Insert("TAGS"),0);
					};
				};
				//
			}
			else
			{
				UpperAlarm->setText("0");
				UpperWarning->setText("0");
				LowerWarning->setText("0");
				LowerAlarm->setText ("0");
				//
				//uaenable int2, uwenable int2, lwenable int2, laenable int2
				//
				LAEnabled->setChecked (0);
				LWEnabled->setChecked (0);
				UWEnabled->setChecked (0);
				UAEnabled->setChecked (0);
				//
				//
                QString cmd = "update TAGS set TAG='"+Name->currentText()+"'where NAME='"+ SPName +"';";
                //QString cmd = "insert into TAGS values ('"+SPName+"','"+Name->currentText()+"'"TAG_VALS"'"+ReceipeName+"',1,0,'','');";
				GetConfigureDb()->DoExec(0,cmd,0);
				//
				// create the entry in the current values database
				// 
				//GetCurrentDb()->DoExec(0,"insert into TAGS_DB values('" + SPName +"','" + Name->currentText() +
				//"'," + DATETIME_EPOCH + ",0,0,0,0,0,0,0,0);" ,0); // create the tag entry into the current values
				//
				// amend the database table
				// it does not matter that there may infact already be a column existing 
				//
				#ifdef USING_POSTGRES 
				cmd = "ALTER TABLE " + SPName + " ADD COLUMN \"" +  Name->currentText().lower() + "\"  float8;";
				#endif
				#ifdef USING_INTERBASE 
				cmd = "ALTER TABLE " + SPName + " ADD COLUMN \"" +  Name->currentText().lower() + "\"  float;";
				#endif
				#ifdef USING_GARRET
				//TODO fix 15-05-05
				//cmd = "alter table " + SPName + " ADD COLUMN \"" +  Name->currentText().lower() + "\"  float;";
				#endif

				//GetResultDb()->DoExec(0,cmd,0);
				//
			};

			ButtonState (true);
			//
		};
		break;
		case tApply:
		ButtonState (true);
		default:
		GetConfigureDb ()->DoneExec (this);	// whatever it was ignore the return
		break;
	};
};

