/*
 *                         IndigoSCADA
 *
 *   This software and documentation are Copyright 2002 to 2014 Enscada 
 *   Limited and its licensees. All rights reserved. See file:
 *
 *                     $HOME/LICENSE 
 *
 *   for full copyright notice and license terms. 
 *
 */

#include <qt.h>
#include "IndentedTrace.h"
#include "general_defines.h"
#include "utilities.h"
#include "buttongroups.h"
#include <process.h>
#include "..\ui\logout.xpm"
#include "..\ui\start.xpm"
#include "..\ui\quit.xpm"
#include "..\ui\computer.xpm"
#include "inifile.h"
#include "general_defines.h"

static QString HomeDirectory = (const char*) 0;
static QString ProjectDirectory = (const char*) 0; //apa 04-12-2020

void SetScadaHomeDirectory(void) 
{ 
	#ifdef WIN32
	
	char path[_MAX_PATH];
	
	path[0] = '\0';
	if(GetModuleFileName(NULL, path, _MAX_PATH))
	{
		*(strrchr(path, '\\')) = '\0';        // Strip \\filename.exe off path
		*(strrchr(path, '\\')) = '\0';        // Strip \\bin off path

		HomeDirectory = path;
    }
		
	#else //UNIX

	char path[256];

	strcpy(path, (const char*)s);
	
	*(strrchr(path, '/')) = '\0';        // Strip /filename.exe off path
	*(strrchr(path, '/')) = '\0';        // Strip /bin off path

	HomeDirectory = path;

	#endif
}

void SetScadaProjectDirectory(void) //apa 04-12-2020
{ 
	#ifdef WIN32
	
	char path[_MAX_PATH];
	
	path[0] = '\0';
	if(GetModuleFileName(NULL, path, _MAX_PATH))
	{
		*(strrchr(path, '\\')) = '\0';        // Strip \\filename.exe off path
		*(strrchr(path, '\\')) = '\0';        // Strip \\bin off path
		
		QString ini_file = QString(path) + "\\bin\\project.ini";
		Inifile iniFile((const char*)ini_file);

		if(iniFile.find("path","project_directory"))
		{
			QString dir;
			dir = iniFile.find("path","project_directory");
			ProjectDirectory = dir;
		}
		else
		{
			//initialize default directory in project.ini
			FILE * fp = fopen((const char*)ini_file,"w+");
			fprintf(fp, "[project_directory]\n");
			fflush(fp);
			fprintf(fp, "path=%s\\project\n", path);
			fflush(fp);
			fclose(fp);

			if(iniFile.find("path","project_directory"))
			{
				QString dir;
				dir = iniFile.find("path","project_directory");
				ProjectDirectory = dir;
			}
		}
    }
		
	#else //UNIX

	char path[256];

	strcpy(path, (const char*)s);
	
	*(strrchr(path, '/')) = '\0';        // Strip /filename.exe off path
	*(strrchr(path, '/')) = '\0';        // Strip /bin off path

	QString ini_file = path + "\\bin\\project.ini";
	Inifile iniFile((const char*)ini_file);

	if(iniFile.find("path","project_directory"))
	{
		QString dir;
		dir = iniFile.find("path","project_directory");
        ProjectDirectory = dir;
	}
	else
	{
		//initialize default directory in project.ini
		FILE * fp = fopen((const char*)ini_file,"w+");
		fprintf(fp, "[project_directory]\n");
		fflush(fp);
		fprintf(fp, "path=%s\\project\n", path);
		fflush(fp);
		fclose(fp);

		if(iniFile.find("path","project_directory"))
		{
			QString dir;
			dir = iniFile.find("path","project_directory");
			ProjectDirectory = dir;
		}
	}

	#endif
}

const QString & GetScadaHomeDirectory() { return HomeDirectory;};
const QString & GetScadaProjectDirectory() { return ProjectDirectory;}; //apa 04-12-2020

void ButtonsGroups::WriteLog(char* pMsg)
{
	// write error or other information into log file
	//::EnterCriticalSection(&myCS);
//	try
//	{
		SYSTEMTIME oT;
		::GetLocalTime(&oT);
		FILE* pLog = fopen(pLogFile,"a");
		fprintf(pLog,"%02d/%02d/%04d, %02d:%02d:%02d\n    %s\n",oT.wMonth,oT.wDay,oT.wYear,oT.wHour,oT.wMinute,oT.wSecond,pMsg); 
		fclose(pLog);
//	} catch(...) {}
	//::LeaveCriticalSection(&myCS);
}

// helper functions

BOOL ButtonsGroups::StartProcess(int nIndex) 
{ 
	// start a process with given index
	STARTUPINFO startUpInfo = { sizeof(STARTUPINFO),NULL,"",NULL,0,0,0,0,0,0,0,STARTF_USESHOWWINDOW,0,0,NULL,0,0,0};  

	char pItem[501];

	sprintf(pItem,"Process%d\0",nIndex);

	char pCommandLine[501];

	pCommandLine[0] ='\0';

	GetPrivateProfileString(pItem,"CommandLine","",pCommandLine,nBufferSize,pInitFile);
	
	if(strlen(pCommandLine)>4)
	{
		char pUserInterface[501];
		
		GetPrivateProfileString(pItem,"UserInterface","N",pUserInterface,nBufferSize,pInitFile);

		BOOL bUserInterface = (pUserInterface[0]=='y'||pUserInterface[0]=='Y'||pUserInterface[0]=='1')?TRUE:FALSE;

		char CurrentDesktopName[512];

		// set the correct desktop for the process to be started
		if(bUserInterface)
		{
			//startUpInfo.wShowWindow = SW_SHOW;
			startUpInfo.wShowWindow = SW_SHOWMINIMIZED;
			startUpInfo.lpDesktop = NULL;
		}
		else
		{
			HDESK hCurrentDesktop = GetThreadDesktop(GetCurrentThreadId());
			DWORD len;
			GetUserObjectInformation(hCurrentDesktop,UOI_NAME,CurrentDesktopName,MAX_PATH,&len);
			startUpInfo.wShowWindow = SW_HIDE;
			startUpInfo.lpDesktop = CurrentDesktopName;
		}

		// create the process

		char pWorkingDir[501];

		GetPrivateProfileString(pItem,"WorkingDir","",pWorkingDir,nBufferSize,pInitFile);

		if(CreateProcess(NULL,pCommandLine,NULL,NULL,TRUE,NORMAL_PRIORITY_CLASS,NULL,strlen(pWorkingDir)==0?NULL:pWorkingDir,&startUpInfo,&pProcInfo[nIndex]))
		{
			char pPause[501];
			GetPrivateProfileString(pItem,"PauseStart","100",pPause,nBufferSize,pInitFile);
			Sleep(atoi(pPause));
			return TRUE;
		}
		else
		{
			long nError = GetLastError();
			char pTemp[121];
			sprintf(pTemp,"Failed to start program '%s', error code = %d", pCommandLine, nError); 
			WriteLog(pTemp);
			return FALSE;
		}
	}
	else 
	{
		char pProcessName[501];

		pProcessName[0] ='\0';

		GetPrivateProfileString(pItem,"Process","",pProcessName,nBufferSize,pInitFile);

		if(strlen(pProcessName)>4)
		{
			QString hm_dir = GetScadaHomeDirectory();
			strcpy(pCommandLine, (const char*)hm_dir);
			strcat(pCommandLine, "\\bin\\");
			strcat(pCommandLine, pProcessName);
						
			char pUserInterface[501];
			
			GetPrivateProfileString(pItem,"UserInterface","N",pUserInterface,nBufferSize,pInitFile);

			BOOL bUserInterface = (pUserInterface[0]=='y'||pUserInterface[0]=='Y'||pUserInterface[0]=='1')?TRUE:FALSE;

			char CurrentDesktopName[512];

			// set the correct desktop for the process to be started
			if(bUserInterface)
			{
				//startUpInfo.wShowWindow = SW_SHOW;
				startUpInfo.wShowWindow = SW_SHOWMINIMIZED;
				startUpInfo.lpDesktop = NULL;
			}
			else
			{
				HDESK hCurrentDesktop = GetThreadDesktop(GetCurrentThreadId());
				DWORD len;
				GetUserObjectInformation(hCurrentDesktop,UOI_NAME,CurrentDesktopName,MAX_PATH,&len);
				startUpInfo.wShowWindow = SW_HIDE;
				startUpInfo.lpDesktop = CurrentDesktopName;
			}

			// create the process

			char pWorkingDir[501];

			GetPrivateProfileString(pItem,"WorkingDir","",pWorkingDir,nBufferSize,pInitFile);

			if(CreateProcess(NULL,pCommandLine,NULL,NULL,TRUE,NORMAL_PRIORITY_CLASS,NULL,strlen(pWorkingDir)==0?NULL:pWorkingDir,&startUpInfo,&pProcInfo[nIndex]))
			{
				char pPause[501];
				GetPrivateProfileString(pItem,"PauseStart","100",pPause,nBufferSize,pInitFile);
				Sleep(atoi(pPause));
				return TRUE;
			}
			else
			{
				long nError = GetLastError();
				char pTemp[121];
				sprintf(pTemp,"Failed to start program '%s', error code = %d", pCommandLine, nError); 
				WriteLog(pTemp);
				return FALSE;
			}
		}
		else
		{
			return FALSE;
		}
	}
}

void ButtonsGroups::EndProcess(int nIndex)
{	
	// end a program started by the service
	if(pProcInfo[nIndex].hProcess)
	{
		char pItem[501];
		sprintf(pItem,"Process%d\0",nIndex);
		char pPause[501];
		GetPrivateProfileString(pItem,"PauseEnd","100",pPause,nBufferSize,pInitFile);
		int nPauseEnd = atoi(pPause);
		// post a WM_QUIT message first
		PostThreadMessage(pProcInfo[nIndex].dwThreadId,WM_QUIT,0,0);
		// sleep for a while so that the process has a chance to terminate itself
		::Sleep(nPauseEnd>0?nPauseEnd:50);
		// terminate the process by force
		TerminateProcess(pProcInfo[nIndex].hProcess,0);
		pProcInfo[nIndex].hProcess = 0;
	}
}

void WorkerProc(void* pParam)
{
	ButtonsGroups* p = (ButtonsGroups*)pParam;

	p->ProcessRestarter();
}

void ButtonsGroups::ProcessRestarter()
{
	int nCheckProcessSeconds = 0;

	char pCheckProcess[500+1];

	GetPrivateProfileString("Settings","CheckProcess","0",pCheckProcess, 500,pInitFile);

	int nCheckProcess = atoi(pCheckProcess);

	if(nCheckProcess>0)
	{
		nCheckProcessSeconds = nCheckProcess*60;
	}
	else
	{
		GetPrivateProfileString("Settings","CheckProcessSeconds","600",pCheckProcess, 500,pInitFile);
		nCheckProcessSeconds = atoi(pCheckProcess);
	}

	while(nCheckProcessSeconds>0)
	{
		::Sleep(1000*nCheckProcessSeconds);

		for(int i = 0;i < nMaxProcCount;i++)
		{
			if(pProcInfo[i].hProcess)
			{
				char pItem[500+1];

				sprintf(pItem,"Process%d\0",i);

				char pRestart[500+1];

				GetPrivateProfileString(pItem,"Restart","No",pRestart,500,pInitFile);

				if(pRestart[0]=='Y'||pRestart[0]=='y'||pRestart[0]=='1')
				{
					DWORD dwCode;

					if(::GetExitCodeProcess(pProcInfo[i].hProcess, &dwCode))
					{
						if(dwCode!=STILL_ACTIVE)
						{
							if(StartProcess(i))
							{
								char pTemp[121];
								sprintf(pTemp, "Restarted process %d", i);
								WriteLog(pTemp);
							}
						}
					}
					else
					{
						long nError = GetLastError();
						char pTemp[121];
						sprintf(pTemp, "GetExitCodeProcess failed, error code = %d", nError);
						WriteLog(pTemp);
					}
				}
			}
		}
	}
}


/*
 * Constructor
 *
 * Creates all child widgets of the ButtonGroups window
 */

ButtonsGroups::ButtonsGroups( QWidget *parent, const char *name )
    : QWidget( parent, name ),nBufferSize(500),nMaxProcCount(127),started(false)
{
    memset(&pProcInfo, 0, sizeof(pProcInfo));
	
	// Create Widgets which allow easy layouting
    QVBoxLayout *vbox = new QVBoxLayout( this, 11, 6 );
    QHBoxLayout *box1 = new QHBoxLayout( vbox );
    QHBoxLayout *box2 = new QHBoxLayout( vbox );

    // ------- first group

    /*

    // Create an exclusive button group
    QButtonGroup *bgrp1 = new QButtonGroup( 1, QGroupBox::Horizontal, "Button Group 1 (exclusive)", this);
    box1->addWidget( bgrp1 );
    bgrp1->setExclusive( TRUE );

    // insert 3 radiobuttons
    QRadioButton *rb11 = new QRadioButton( "&Radiobutton 1", bgrp1 );
    rb11->setChecked( TRUE );
    (void)new QRadioButton( "R&adiobutton 2", bgrp1 );
    (void)new QRadioButton( "Ra&diobutton 3", bgrp1 );

    // ------- second group

    // Create a non-exclusive buttongroup
    QButtonGroup *bgrp2 = new QButtonGroup( 1, QGroupBox::Horizontal, "Button Group 2 (non-exclusive)", this );
    box1->addWidget( bgrp2 );
    bgrp2->setExclusive( FALSE );

    // insert 3 checkboxes
    (void)new QCheckBox( "&Checkbox 1", bgrp2 );
    QCheckBox *cb12 = new QCheckBox( "C&heckbox 2", bgrp2 );
    cb12->setChecked( TRUE );
    QCheckBox *cb13 = new QCheckBox( "Triple &State Button", bgrp2 );
    cb13->setTristate( TRUE );
    cb13->setChecked( TRUE );

    // ------------ third group

    // create a buttongroup which is exclusive for radiobuttons and non-exclusive for all other buttons
    QButtonGroup *bgrp3 = new QButtonGroup( 1, QGroupBox::Horizontal, "Button Group 3 (Radiobutton-exclusive)", this );
    box2->addWidget( bgrp3 );
    bgrp3->setRadioButtonExclusive( TRUE );

    // insert three radiobuttons
    rb21 = new QRadioButton( "Rad&iobutton 1", bgrp3 );
    rb22 = new QRadioButton( "Radi&obutton 2", bgrp3 );
    rb23 = new QRadioButton( "Radio&button 3", bgrp3 );
    rb23->setChecked( TRUE );

    // insert a checkbox...
    state = new QCheckBox( "E&nable Radiobuttons", bgrp3 );
    state->setChecked( TRUE );
    // ...and connect its SIGNAL clicked() with the SLOT slotChangeGrp3State()
    connect( state, SIGNAL( clicked() ), this, SLOT( slotChangeGrp3State() ) );
	*/

    // ------------ fourth group

    // create a groupbox which layouts its childs in a columns
    QGroupBox *bgrp4 = new QButtonGroup( 1, QGroupBox::Horizontal, "Control board", this );
    box1->addWidget( bgrp4 );

	QString start_str = QString("&Start ") + QString(""SYSTEM_NAME"");

    // insert four pushbuttons...
    QPushButton *tb1 = new QPushButton( start_str, bgrp4, "push" );
	tb1->setOn(FALSE);

	connect( tb1, SIGNAL( clicked() ), this, SLOT( slotStartProcesses() ) );

	QString stop_str = QString("Stop &") + QString(""SYSTEM_NAME"");

    // now make the second one a toggle button
    QPushButton *tb2 = new QPushButton( stop_str, bgrp4, "toggle" );
    //tb2->setToggleButton( TRUE );
    tb2->setOn( FALSE );

	connect( tb2, SIGNAL( clicked() ), this, SLOT( slotStopProcesses() ) );

    // ... and make the third one a flat button
    QPushButton *tb3 = new QPushButton( "Set project folder", bgrp4, "flat" );
	tb3->setOn(FALSE);
    //tb3->setFlat(TRUE);

	connect( tb3, SIGNAL( clicked() ), this, SLOT( slotSelectFolder() ) );

	textLabel1 = new QLabel( this, "textLabel1" );
    textLabel1->setGeometry( QRect( 30, 190, 2000, 40 ) );

	char file[_MAX_PATH];
	strcpy(file, "Project folder:\n");
	strcat(file, (const char*)GetScadaProjectDirectory());
	textLabel1->setText(file);

    // .. and the fourth a button with a menu
	/*
    QPushButton *tb4 = new QPushButton( "Popup Button", bgrp4, "popup" );
    QPopupMenu *menu = new QPopupMenu(tb4);
    menu->insertItem("Item1", 0);
    menu->insertItem("Item2", 1);
    menu->insertItem("Item3", 2);
    menu->insertItem("Item4", 3);
    tb4->setPopup(menu);
	*/

	/////////////////////////////////////////////////////////////////////////////////

	strcpy(pInitFile, (const char*)GetScadaProjectDirectory()); //apa 04-12-2020

	strcat(pInitFile, "\\manager.ini");
		
	pLogFile[0] = '\0';

	#ifdef WIN32
	if(GetModuleFileName(NULL, pLogFile, _MAX_PATH))
	{
		*(strrchr(pLogFile, '\\')) = '\0';        // Strip \\filename.exe off path
		*(strrchr(pLogFile, '\\')) = '\0';        // Strip \\bin off path
    }
	#endif

	strcat(pLogFile, "\\logs\\manager.log");


	// start a worker thread to check for dead programs (and restart if necessary)
	if(_beginthread(WorkerProc, 0, (void*)this)==-1)
	{
		long nError = GetLastError();
		char pTemp[121];
		sprintf(pTemp, "_beginthread failed, error code = %d", nError);
		WriteLog(pTemp);
	}

	//automatic start
	char pAutomaticStart[500+1];

	GetPrivateProfileString("Settings","AutomaticStart","0",pAutomaticStart, 500,pInitFile);

	int isAutoStart = atoi(pAutomaticStart);

	if(isAutoStart > 0)
	{
		if(started == false)
		{
			started = true;

			for(int i = 0;i < 10; i++)
			{
				StartProcess(i);
			}
		}
	}
}

void ButtonsGroups::slotStartProcesses()
{
	if(started == false)
	{
		started = true;

		for(int i = 0;i < 10; i++)
		{
			StartProcess(i);
		}
	}
}

void ButtonsGroups::slotStopProcesses()
{
	if(started == true)
	{
		started = false;
		
		for(int i = 10; i >= 0; i--)
		{
			EndProcess(i);
		}
	}
}

void ButtonsGroups::slotSelectFolder()
{
	QFileDialog dialog;
	dialog.setMode(QFileDialog::DirectoryOnly);
	dialog.setCaption(tr("Choose project folder"));

	int res = dialog.exec();
		
	if(res)
	{
		QString new_dir = dialog.dirPath();

		char new_folder[_MAX_PATH];

		strcpy(new_folder, (const char*)new_dir);

		char *ptr = new_folder;

		while(*ptr)
		{
			if(*ptr=='/') 
			{
				*ptr='\\';
			}

			ptr++;
		}
		
		char ini_file[_MAX_PATH];
			
		ini_file[0] = '\0';
		if(GetModuleFileName(NULL, ini_file, _MAX_PATH))
		{
			*(strrchr(ini_file, '\\')) = '\0';        // Strip \\filename.exe off path
			*(strrchr(ini_file, '\\')) = '\0';        // Strip \\bin off path
			
			strcat(ini_file, "\\bin\\project.ini");
			

			WritePrivateProfileString("project_directory", "path", new_folder, ini_file);

			char file[_MAX_PATH];
			strcpy(file, "Project folder:\n");
			strcat(file, new_folder);
			textLabel1->setText(file);

			SetScadaProjectDirectory();

			strcpy(pInitFile, (const char*)GetScadaProjectDirectory());

			strcat(pInitFile, "\\manager.ini");
		}
	}
}
