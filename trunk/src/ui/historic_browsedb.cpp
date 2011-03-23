/*
 *                         IndigoSCADA
 *
 *   This software and documentation are Copyright 2002 to 2009 Enscada 
 *   Limited and its licensees. All rights reserved. See file:
 *
 *                     $HOME/LICENSE 
 *
 *   for full copyright notice and license terms. 
 *
 */

/*
*Header For: Browser database
*Purpose: Broeser interface for database
*/
#include "historic_browsedb.h"
#include "IndentedTrace.h"
#include "AckAlarmDlg.h"
#include "server.xpm"
#include "mini-datawidget.xpm"
#include "table_plus.xpm"
#include "db.xpm"
#include "Tag.xpm"
#include "sql.xpm"
#include "clear.xpm"
#include "save.xpm"
#include "clock.xpm"
#include "table_arrow.xpm"
#include "quit.xpm"
#include "pencil.xpm"
#include "helpicon.xpm" 

#define MESSAGES_LIMIT 256

//
/*
*Function:HistoricBrowsedb
*Inputs:none
*Outputs:none
*Returns:none
*/
HistoricBrowsedb::HistoricBrowsedb(HistoricDbDict& dct) : QMainWindow(0,0,WDestructiveClose),	
	maxItems(150),
	pSm(NULL),
	pMessages(NULL),
	List(NULL),
	db_table(NULL),
	records_fetched(0),
	delete_blob_counter(0),
	MoreOldRecord(0),
	MoreRecentRecord(0)
{
	IT_IT("HistoricBrowsedb::HistoricBrowsedb");
	//
	db_dictionary = dct;

	HistoricDbDict::iterator it = db_dictionary.begin();

	//connect to online databases
	for(; !(it == db_dictionary.end());it++)
	{
		connect ((*it).second,
		SIGNAL (TransactionDone (QObject *, const QString &, int, QObject*)), this,
		SLOT (QueryResponse (QObject *, const QString &, int, QObject*)));	
	};
	//
	//
	QSplitter* pSv = new QSplitter(QSplitter::Vertical,this); // => horizontal bar
	pSv->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	setCentralWidget(pSv);

	QValueList<int> l;
	l.append(height()*39/40);
	l.append(height()*1/40);
	pSv->setSizes(l);

	// create the splitter
	pSm =  new QSplitter(QSplitter::Horizontal, pSv); // => vertical bar
	pSm->setFrameStyle(QFrame::Panel | QFrame::Sunken);

	pMessages = new QListBox(pSv); // we now have the list views
	//
	//
	//////////////Toolbar//////////////////////////////////////////////
	#include "first_cmp.xpm"
	#include "before_cmp.xpm"
	#include "next_cmp.xpm"
	#include "last_cmp.xpm"
	pToolBarBrowse = new QToolBar("Browse database",this,QMainWindow::Top);
	(void)  new QToolButton(QPixmap((const char **)first_cmp),tr("First"),0,this,SLOT(first()),pToolBarBrowse, "First");
	(void)  new QToolButton(QPixmap((const char **)before_cmp),tr("Previous"),0,this,SLOT(before()),pToolBarBrowse, "Previous");
	(void)  new QToolButton(QPixmap((const char **)next_cmp),tr("Next"),0,this,SLOT(next()),pToolBarBrowse, "Next");
	(void)  new QToolButton(QPixmap((const char **)last_cmp),tr("Last"),0,this,SLOT(last()),pToolBarBrowse, "Last");
	///////////end////////Toolbar//////////////////////////////////////
	//
	// Add the selection dialog
	List = new QListView(pSm);
	List->setFrameStyle(QFrame::Panel | QFrame::Sunken);
	List->addColumn(tr("Online historical databases"));
	List->setShowSortIndicator(true);
	List->setSorting(0);

	connect (List,SIGNAL (mouseButtonPressed(int, QListViewItem*, const QPoint &, int)),
	this, SLOT (mouseButtonPressed(int, QListViewItem*, const QPoint &, int)));

	connect (List,SIGNAL (returnPressed(QListViewItem*)),
	this, SLOT (returnPressed(QListViewItem*)));

	DateTime = false;
	//
	//QValueList<int> l;
	l.clear();
	l.append(width()  / 6);
	l.append(width()*5 / 6);
	pSm->setSizes(l);
	//setCentralWidget(pSm);
	//
	QPopupMenu *file = new QPopupMenu(this);
	file->insertItem(QPixmap((const char **)quit_xpm), tr("Close"),this,SLOT(Close()));
	menuBar()->insertItem(tr("&File"),file);
	//
	//
	//
	QPopupMenu *options = new QPopupMenu(this);
	menuBar()->insertItem(tr("&Options"),options);

	QPopupMenu *help = new QPopupMenu(this);
	help->insertItem(QPixmap((const char **)helpicon), tr("Help..."),this,SLOT(Help()));
	menuBar()->insertItem(tr("&Help"),help); 

	QAction *date_time_menu;
	date_time_menu = new QAction( tr("View Date/Time"), QPixmap((const char **)clock_icon), tr("&View Date/Time"), 0, this, 0, TRUE );
	
	connect( date_time_menu, SIGNAL( toggled(bool) ), this, SLOT( DataTimeOn(bool) ) );
	date_time_menu->setOn( FALSE );

	date_time_menu->addTo(options);

	options->insertSeparator();
	options->insertItem(QPixmap((const char **)pencil_xpm),tr("Maximum records..."),this,SLOT(getMaxItems()));


	//QPopupMenu *sql_command = new QPopupMenu(this);
	//sql_command->insertItem(QPixmap((const char **)sql),tr("SQL"),this,SLOT(ExecuteSQLCommand()));
	//QPopupMenu *admin_menu = new QPopupMenu( this );
	//sql_command->insertItem(QPixmap((const char **)table_icon),tr("&Admin Database..."), admin_menu);
	//admin_menu->insertItem(QPixmap((const char **)clear_xpm), tr("&Purge Results Database..."),this,SLOT(Purge()));
	//admin_menu->insertItem(QPixmap((const char **)save_xpm), tr("&Backup Database..."),this,SLOT(Backup()));
	//menuBar()->insertItem(tr("&Command"), sql_command);

	//
	setCaption(tr("Historical database management"));
	//
	List->clear();
		
	it = db_dictionary.begin();
	
	for(; !(it == db_dictionary.end());it++)
	{
		(*it).second->DoExec(this,"show;",tList);
	};
	//
	QPixmap icon((const char **)server_xpm);
	setIcon(icon);
	//
};

/*
*Function:
*Inputs:none
*Outputs:none
*Returns:none
*/
void HistoricBrowsedb::QueryResponse (QObject *p,const QString &, int id, QObject*caller)
{
	if(p != this) return;

	IT_IT("HistoricBrowsedb::QueryResponse");

	HISTORIC_DB* Caller = (HISTORIC_DB*)caller;

	switch(id)
	{
		case tResults:
		{
			QSTransaction &t = Caller->CurrentTransaction();
						
			int n = Caller->GetNumberResults();
			if(n > 0)
			{
				if(n > maxItems)
				{
					statusBar()->message(tr("Show only the first ") + QString::number(maxItems) + tr(" records"));

					n = maxItems;
				}
				
				QStringList l;
				int j = Caller->GetFieldNames(l);
				if(j)
				{ 
					if(db_table)
					{
					  delete db_table;
					  db_table = NULL;
					}
					db_table = new DbTable(n, j, pSm, t.Data1);
					db_table->setDatabase(Caller);

					connect (db_table, SIGNAL(valueChanged(int,int)),
					this, SLOT (valueChanged(int,int)));
				
					for(int i = 0; i < j; i++)
					{    
						(db_table->horizontalHeader())->setLabel( i, l[i] );
					};
										
					for(int k = 0; k < n && k < maxItems; k++,Caller->FetchNext())
					{
						for(unsigned i = 0; i < l.count(); i++)
						{
							if(DateTime)
							{
								if((l[i]).contains("TIME"))
								{
									db_table->setText( k, i, Caller->GetIsoDateString(l[i]) );	
								}
								else
								{
									db_table->setText( k, i, Caller->GetString(l[i]) );
								}
								db_table->adjustColumn(i);
							}
							else
							{
								db_table->setText( k, i, Caller->GetString(l[i]) );
								db_table->adjustColumn(i);
							}

							if(l[i] == t.Data2) //we get indexed column
							{
								timestamp = _atoi64((const char*)(Caller->GetString(l[i]))); 

								if(k == 0)
								{
									MoreOldRecord = timestamp;
									MoreRecentRecord = timestamp;
								}
								else
								{
									if(timestamp > MoreRecentRecord)
									{
										MoreRecentRecord = timestamp;
									}

									if(timestamp < MoreOldRecord)
									{
										MoreOldRecord = timestamp;
									}
								}
							}
						};
					};
					db_table->show();
				};
			}
			else
			{
				if(db_table)
				{
				  delete db_table;
				  db_table = NULL;
				}
				db_table = new DbTable(1, 1, pSm, t.Data1);
				db_table->setDatabase(Caller);

				connect (db_table, SIGNAL(valueChanged(int,int)),
				this, SLOT (valueChanged(int,int)));
			}

			ShowDbMessages(Caller);
		
			statusBar()->message(tr(""));
			this->setCursor(QCursor(ArrowCursor));
		};
		break;
		/*
		case tPurge:
		{
			QSTransaction &t = Caller->CurrentTransaction();
			
			if(t.Data1 == GetHistoricResultDb()->GetName())
			{
				int n = Caller->GetNumberResults();
				if(n > 0)
				{
					bool ok = 0;
					// 
					int back = QInputDialog::getInteger(tr("Purge Results Database"), tr("Last number of minutes to keep"),60,1,600,1,&ok,this);
					//
					if(ok)
					{
						//QDateTime dt = QDateTime::currentDateTime().addDays(-back); 
						QDateTime dt = QDateTime::currentDateTime().addSecs(-back*60); 
						//
						if(YESNO(tr("Purge Results Databases"),tr("Purge Are You Sure ?")) == 0)
						{
							for(int i = 0; i < n; i++,Caller->FetchNext())
							{
								QString cmd = "delete from " + Caller->GetString("NAME") + " where TIMEDATE < " +  
								QDATE_TIME_ISO_DATE(dt) + ";";
								GetHistoricResultDb()->DoExec(0,cmd,0);
								DOAUDIT(tr("Purging Results For ") + Caller->GetString("NAME"));
							};
						};
						//
						// Now purge the alarm log
						GetHistoricResultDb()->DoExec(0,"delete from ALARMS where TIMEDATE < " + QDATE_TIME_ISO_DATE(dt) + ";",0);
						DOAUDIT(tr("Purging Alarm Log"));
						//
						// Now purge the events log
						GetHistoricResultDb()->DoExec(0,"delete from EVENTS where TIMEDATE < " + QDATE_TIME_ISO_DATE(dt) + ";",0);
						DOAUDIT(tr("Purging Event Log"));
					}
				};

				statusBar()->message(tr("Result db purged"));
			}
			else if(t.Data1 == GetConfigureDb()->GetName())
			{
				statusBar()->message(tr("No action performed"));
			}
			else if(t.Data1 == GetCurrentDb()->GetName())
			{
				statusBar()->message(tr("No action performed"));
			}

			ShowDbMessages(Caller);
			this->setCursor(QCursor(ArrowCursor));
		};
		break;
		*/
		case tDeleteBlobEnd:
		{
			delete_blob_counter++;

			ShowDbMessages(Caller);

			if(delete_blob_counter == records_fetched*2)
			{
				statusBar()->message(tr(""));
				this->setCursor(QCursor(ArrowCursor));
			}
		}
		break;
		case tBackupDb:
		{
			ShowDbMessages(Caller);
			statusBar()->message(tr(""));
			this->setCursor(QCursor(ArrowCursor));
		}
		break;
		case tList:
		{
			// build the database list - build as a tree
			
			QString db_name = Caller->GetName();
			     
			QListViewItem* item = new QListViewItem(List,db_name); 
			item->setPixmap( 0, QPixmap(db_xpm) );

			int n = Caller->GetNumberResults();
			
			for(int k = 0; k < n; k++, Caller->FetchNext())
			{
				QListViewItem*pi =  new QListViewItem(item, Caller->GetString("TABLE_NAME"));
				pi->setPixmap( 0, QPixmap(table_plus_xpm) );
			};

			List->triggerUpdate();
			Caller->AcnoledgeError();
		};
		break;
		case tValueChanged:
		{
			QString index_field_name = (const char*) 0;
			unsigned int index_field_col = 0;
			bool is_index_ascii = false;

			QSTransaction &t = Caller->CurrentTransaction();
			QString updatedColumnName = (db_table->horizontalHeader())->label(t.Data2.toInt());
			
			unsigned n = Caller->GetNumberResults();
						
			if(n > 0)
			{						
				for(unsigned int i = 0; i < n; i++,Caller->FetchNext())
				{
					if(Caller->GetString("FIELD_INDEX") == "INDEXED")
					{
						index_field_name = Caller->GetString("FIELD_NAME");
						index_field_col = i;

						if(Caller->GetString("FIELD_TYPE") == "asciiz")
						{
							is_index_ascii = true;
						}
					}
				}

				Caller->GotoBegin();

				for(i = 0; i < n; i++,Caller->FetchNext())
				{
					if(Caller->GetString("FIELD_NAME") == updatedColumnName)
					{
						if(index_field_name)
						{
							QString cmd = QString("update ") + db_table->name() + QString(" set ") + Caller->GetString("FIELD_NAME") + QString("=");

							if(Caller->GetString("FIELD_TYPE") == "asciiz")
							{
								cmd += QString("'") + db_table->text(t.Data1.toInt(), t.Data2.toInt()) + QString("'");
							}
							else
							{
								cmd += db_table->text(t.Data1.toInt(), t.Data2.toInt());
							}
							
							cmd += QString(" where ") + index_field_name + QString("="); 
								
							if(is_index_ascii)
							{
								cmd += QString("'") + db_table->text(t.Data1.toInt(), index_field_col) + QString("'");
							}
							else
							{
								cmd += db_table->text(t.Data1.toInt(), index_field_col); 
							}

							cmd += ";"; 
								
							Caller->DoExec(this,cmd,tUpdateDb);
						}
					}
				}
			}
		}
		break;
		case tUpdateDb:
		{
			ShowDbMessages(Caller);
		}
		break;
		case tBrowse:
		{
			QString index_field_name = (const char*) 0;
			bool is_index_ascii = false;

			QSTransaction &t = Caller->CurrentTransaction();
						
			unsigned n = Caller->GetNumberResults();
						
			if(n > 0)
			{						
				for(unsigned int i = 0; i < n; i++,Caller->FetchNext())
				{
					if(Caller->GetString("FIELD_INDEX") == "INDEXED")
					{
						index_field_name = Caller->GetString("FIELD_NAME");
						
						if(Caller->GetString("FIELD_TYPE") == "asciiz")
						{
							is_index_ascii = true;
						}
					}
				}

				if(t.Data1 == "first")
				{
				   	QListViewItem* item = List->currentItem();
					if(!item->childCount())
					{
						HistoricDbDict::iterator i = db_dictionary.find(((QListViewItem *)item->parent())->text(0));
									
						if(!(i == db_dictionary.end()))
						{
							QString table = item->text(0);
							QString cmd = "select * from " + table + " order by " + index_field_name + " asc limit " + QString::number(maxItems) + ";"; 
							(*i).second->DoExec(this, cmd, tResults, table, index_field_name);
						}
					}

				}
				else if(t.Data1 == "before")
				{
					QListViewItem* item = List->currentItem();
					if(item)
					{
						if(!item->childCount())
						{
							HistoricDbDict::iterator i = db_dictionary.find(((QListViewItem *)item->parent())->text(0));
								
							if(!(i == db_dictionary.end()))
							{
								//if(is_index_ascii == false)
								//{
									char buffer[20];
									_i64toa(MoreOldRecord, buffer, 10);
									
									QString table = item->text(0);
									QString cmd = "select * from " + table + " where (" + index_field_name + "<" +
									QString(buffer) + ") order by " + index_field_name + " desc limit " + QString::number(maxItems) + ";";
									(*i).second->DoExec(this, cmd, tResults, table, index_field_name);
								//}
							}
						}
					}
				}
				else if(t.Data1 == "next")
				{
					QListViewItem* item = List->currentItem();
					if(!item->childCount())
					{
						HistoricDbDict::iterator i = db_dictionary.find(((QListViewItem *)item->parent())->text(0));
							
						if(!(i == db_dictionary.end()))
						{
							//if(is_index_ascii == false)
							//{
								char buffer[20];
								_i64toa(MoreRecentRecord, buffer, 10);

								QString table = item->text(0);
								QString cmd = "select * from " + table + " where (" + index_field_name + ">" +
								QString(buffer) + ") order by " + index_field_name + " asc limit " + QString::number(maxItems) + ";"; 
								(*i).second->DoExec(this, cmd, tResults, table, index_field_name);
							//}
						}
					}
				}
				else if(t.Data1 == "last")
				{
					QListViewItem* item = List->currentItem();
					if(!item->childCount())
					{
						HistoricDbDict::iterator i = db_dictionary.find(((QListViewItem *)item->parent())->text(0));
							
						if(!(i == db_dictionary.end()))
						{
							QString table = item->text(0);
							QString cmd = "select * from " + table + " order by " + index_field_name + " desc limit " + QString::number(maxItems) + ";"; 
							(*i).second->DoExec(this, cmd, tResults, table, index_field_name);
						}
					}
				}
			}
		}
		break;
		default:
		break;
	}
};

void HistoricBrowsedb::valueChanged ( int row, int col )
{
	HISTORIC_DB*db  = ((HISTORIC_DB*)db_table->getDatabase()); 
	QString cmd = QString("describe ") + db_table->name() + ";";
	db->DoExec(this,cmd,tValueChanged, QString::number(row), QString::number(col));
}

/*
*Function: Close
*Inputs:none
*Outputs:none
*Returns:none
*/
void HistoricBrowsedb::Close()
{
	IT_IT("HistoricBrowsedb::Close");

	if(List)
	{
		delete List;
		List = NULL;
	}
	if(pMessages)
	{
		delete pMessages;
		pMessages = NULL;
	}

	if(db_table)
	{
	  delete db_table;
	  db_table = NULL;
	}

	if(pSm)
	{
		delete pSm;
		pSm = NULL;
	}
	
	close(true); // close and delete the window
};


void HistoricBrowsedb::mouseButtonPressed( int button, QListViewItem * item, const QPoint & pos, int c  )
{
	IT_IT("HistoricBrowsedb::mouseButtonPressed");
	
	if(button == 1) //left mouse button pressed
	{
		if(item)
		{
			if(!item->childCount()) //it it's a table name
			{
				HistoricDbDict::iterator i = db_dictionary.find(((QListViewItem *)item->parent())->text(0));
				
				if(!(i == db_dictionary.end()))
				{
					QString table = item->text(0);
					QString cmd = "select * from " + table + " limit " + QString::number(maxItems) + ";";  
					(*i).second->DoExec(this, cmd, tResults, table, "index name");
					IT_COMMENT1("Table: %s", (const char *)table);
					statusBar()->message(tr("Fetching Data"));
					this->setCursor(QCursor(WaitCursor));
				}
			}
		}
	}
	else
	{
		if(item) 
		{
			if(item->childCount())//if it's a database name
			{
				QPopupMenu m(List);
				m.insertItem(QPixmap((const char **)sql),tr("Sql"),0); 
				m.insertItem(QPixmap((const char **)save_xpm),tr("Backup Database"),1); 
				//m.insertItem(QPixmap((const char **)clear_xpm),tr("Purge Database"),2); 
				m.insertItem(tr("Cancel"),-1);
				//
				switch(m.exec(QCursor::pos()))
				{
					case 0: 
					{
						HistoricDbDict::iterator i = db_dictionary.find(item->text(0));
						
						if(!(i == db_dictionary.end()))
						{
							bool ok = 0;
							QString msg = tr("Enter SQL Command for ") + (*i).first;
							QString cmdtxt = tr("Command for ") + (*i).first;
							QString cmd = QInputDialog::getText(msg, cmdtxt,QLineEdit::Normal,tr(""),&ok,this);
							if(ok && !cmd.isEmpty())
							{
								(*i).second->DoExec(this, cmd, tResults);
								statusBar()->message(tr("Fetching Data"));
								this->setCursor( QCursor(WaitCursor) );
							}
						}
					};
					break;
					case 1:
					{
						HistoricDbDict::iterator i = db_dictionary.find(item->text(0));
						
						if(!(i == db_dictionary.end()))
						{
							QString msg = tr("Backup Database ") + (*i).first;
							if(YESNO(msg, tr("Backup Are You Sure ?")) == 0)
							{
								QString cmd = QString("backup ") + "'"+ QSBIN_DIR + "\\" +DATETIME_NOW+ ".bak';";
								(*i).second->DoExec(this, cmd, tBackupDb);
								statusBar()->message(tr("Backup database"));
								this->setCursor( QCursor(WaitCursor) );
							}
						}
					}
					break;
					/*
					case 2:
					{
						HistoricDbDict::iterator i = db_dictionary.find(item->text(0));
						
						if(!(i == db_dictionary.end()))
						{
							QString msg = tr("Purge Database ") + (*i).first;
							if(YESNO(msg,tr("Purge Are You Sure ?")) == 0)
							{
								GetConfigureDb()->DoExec(this,"select NAME from SAMPLE;",tPurge, item->text(0));

								statusBar()->message(tr("Purge database"));
								this->setCursor( QCursor(WaitCursor) );
							}
						}
					}
					break;
					*/
					default:
					break;
				};
			}
			else //it is a table name
			{
				QPopupMenu m(List);
				QString desc = tr("Describe table ") + item->text(0);
				m.insertItem(QPixmap((const char **)table_arrow_xpm),desc,0); 
				m.insertItem(tr("Cancel"),-1);
				//
				switch(m.exec(QCursor::pos()))
				{
					case 0: 
					{
						HistoricDbDict::iterator i = db_dictionary.find(((QListViewItem *)item->parent())->text(0));
						
						if(!(i == db_dictionary.end()))
						{
							if(item->text(0) == EMPTY_DATABASE) break;
							
							QString table = item->text(0);
							QString cmd = "describe " + table + ";"; 
							(*i).second->DoExec(this, cmd, tResults, table, "index name");
							statusBar()->message(tr("Fetching Data"));
							this->setCursor( QCursor(WaitCursor) );
						}
					};
					break;
					default:
					break;
				};
			}
		}
	}
}


void HistoricBrowsedb::returnPressed(QListViewItem* item)
{
	IT_IT("HistoricBrowsedb::returnPressed");

	if(item)
	{
		if(!item->childCount())
		{
			QString name = ((QListViewItem *)item->parent())->text(0);
			
			HistoricDbDict::iterator i = db_dictionary.find(name);
			
			if(!(i == db_dictionary.end()))
			{
				QString table = item->text(0);
				QString cmd = "select * from " + table + " limit " + QString::number(maxItems) + ";";  
				(*i).second->DoExec(this, cmd, tResults, table, "index name");
				IT_COMMENT1("Table: %s", (const char *)table);
				statusBar()->message(tr("Fetching Data"));
				this->setCursor( QCursor(WaitCursor) );
			}
		}
		else
		{
			if(item->isOpen())
			{
				List->setOpen(item, FALSE);
			}
			else
			{
				List->setOpen(item, TRUE);
			}
		}
	}
}


void HistoricBrowsedb::DataTimeOn(bool isOn)
{
	if(isOn)
		DateTime = true;
	else
		DateTime = false;
}


void HistoricBrowsedb::first()
{
	QListViewItem* item = List->currentItem();
	if(!item->childCount())
	{
		HistoricDbDict::iterator i = db_dictionary.find(((QListViewItem *)item->parent())->text(0));
					
		if(!(i == db_dictionary.end()))
		{
			QString table = item->text(0);
			QString cmd = QString("describe ") + table + ";";
			(*i).second->DoExec(this, cmd, tBrowse, "first");
			statusBar()->message(tr("Fetching Data"));
			this->setCursor( QCursor(WaitCursor) );
		}
	}
}

void HistoricBrowsedb::before()
{
	QListViewItem* item = List->currentItem();
	if(item)
	{
		if(!item->childCount())
		{
			HistoricDbDict::iterator i = db_dictionary.find(((QListViewItem *)item->parent())->text(0));
				
			if(!(i == db_dictionary.end()))
			{
				QString table = item->text(0);
				QString cmd = QString("describe ") + table + ";";
				(*i).second->DoExec(this, cmd, tBrowse, "before");
				statusBar()->message(tr("Fetching Data"));
				this->setCursor( QCursor(WaitCursor) );
			}
		}
	}
}

void HistoricBrowsedb::next()
{
	QListViewItem* item = List->currentItem();
	if(!item->childCount())
	{
		HistoricDbDict::iterator i = db_dictionary.find(((QListViewItem *)item->parent())->text(0));
			
		if(!(i == db_dictionary.end()))
		{
			QString table = item->text(0);
			QString cmd = QString("describe ") + table + ";";
			(*i).second->DoExec(this, cmd, tBrowse, "next");
			statusBar()->message(tr("Fetching Data"));
			this->setCursor( QCursor(WaitCursor) );
		}
	}
}

void HistoricBrowsedb::last()
{
	QListViewItem* item = List->currentItem();
	if(!item->childCount())
	{
		HistoricDbDict::iterator i = db_dictionary.find(((QListViewItem *)item->parent())->text(0));
			
		if(!(i == db_dictionary.end()))
		{
			QString table = item->text(0);
			QString cmd = QString("describe ") + table + ";";
			(*i).second->DoExec(this, cmd, tBrowse, "last");
			statusBar()->message(tr("Fetching Data"));
			this->setCursor( QCursor(WaitCursor) );
		}
	}
}


void HistoricBrowsedb::ShowDbMessages(QSDatabase* pdb)
{
	//Show messages and errors from client and server

	QString message = pdb->GetDbMessage();
	
	if(pdb->GetErrorMessage() != QString::null)
	{
		message += " Error: " + pdb->GetErrorMessage();
		pdb->AcnoledgeError();
	}
	
	pMessages->insertItem(message);

	if(pMessages->count())
	{
		pMessages->setSelected (pMessages->count() - 1, TRUE );
		pMessages->ensureCurrentVisible();
	}

	if(pMessages->count() > MESSAGES_LIMIT)
	{
		QListBoxItem* p = pMessages->firstItem();
		pMessages->takeItem(p);
		delete p;
	};
}

/*
*Function: getMaxItems
*make a usr entry
*Inputs:none
*Outputs:none
*Returns:none
*/
void HistoricBrowsedb::getMaxItems()
{
	IT_IT("HistoricBrowsedb::getMaxItems");
	
	AckAlarmDlg dlg(this);
	dlg.SetHeading(tr("Maximum records fetched in query"));
	if(dlg.exec())
	{
		maxItems = (dlg.GetComment()).toLong();
	};
};

/*-Function: Help
*Inputs:none
*Outputs:none
*Returns:none
*/
void HistoricBrowsedb::Help() // help
{
	IT_IT("HistoricBrowsedb::Help");
	
	QSHelp("SQL");
};
