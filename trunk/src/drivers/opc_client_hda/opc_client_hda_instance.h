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

#ifndef OPC_CLIENT_HDA__INSTANCE
#define OPC_CLIENT_HDA__INSTANCE

#include "opc_client_hda.h"
#include "IndentedTrace.h"
#include "fifo.h"
#include "fifoc.h"
#include "iec104types.h"
#include "iec_item.h"

void iec_call_exit_handler(int line, char* file, char* reason);

class Opc_client_hda_DriverThread;

class OPC_CLIENT_HDADRV Opc_client_hda_Instance : public DriverInstance 
{
	Q_OBJECT
	//
	//
	enum
	{
		tUnitProperties = 1,tList, tSamplePointProperties, tListUnits
	};
	//
	//
	
//	QStringList SampleList; // list of sample points
	bool fFail;
	QTimer *pTimer; // timer object for driving state machine
	int Retry; // the retry count
	int Countdown; // the countdown track
	int State; // the state machine's state
	int instanceID;
	
	//  
	int Sp; //Current sample point index under measurement
	bool InTick; //tick sentinal
	int OpcItems;
	
	struct  Track
	{
		QString Name;           // name of sample point
		SampleStatistic Stats;  // we track the stats  
		double LastValue;       // the last value
		bool   fSpotValue;        // do we report the last value or the mean of the values over the sample period
		unsigned  SamplePeriod; // how often we sample 
		QDateTime NextSample;
		bool fFailed; // flag if the sample point is in a failed state
		void clear()
		{
			LastValue = 0.0; fSpotValue = false;
			NextSample = QDateTime::currentDateTime();
			fFailed = false;
			Stats.reset();
		}; 
	};
	//
	Track* Values;

	enum // states for the state machine
	{
		STATE_IDLE = 0,
		STATE_READ,
		STATE_WRITE,
		STATE_RESET,
		STATE_FAIL,
		STATE_DONE
	};

	public:
	Opc_client_hda_DriverThread *pConnect;
	fifo_h fifo_control_direction;
	unsigned int msg_sent_in_control_direction;

	//
	Opc_client_hda_Instance(Driver *parent, const QString &name, int instance_id) : 
	DriverInstance(parent,name),fFail(0), Countdown(1), pConnect(NULL),
	State(STATE_RESET),InTick(0),Retry(0),Sp(0),OpcItems(1), Values(NULL),
	ParentDriver(parent),msg_sent_in_control_direction(0), instanceID(instance_id)
	{
		IT_IT("Opc_client_hda_Instance::Opc_client_hda_Instance");
		connect (GetConfigureDb (),
		SIGNAL (TransactionDone (QObject *, const QString &, int, QObject*)), this,
		SLOT (QueryResponse (QObject *, const QString &, int, QObject*)));	// connect to the database

		pTimer = new QTimer(this);
		connect(pTimer,SIGNAL(timeout()),this,SLOT(Tick()));
		pTimer->start(1000); // start with a 1 second timer

		/////////////////////////////////////////////////////////////////////////////
		char fifo_ctr_name[150];
		char str_instance_id[20];
        itoa(instance_id + 1, str_instance_id, 10);
		strcpy(fifo_ctr_name,"fifo_control_direction");
        strcat(fifo_ctr_name, str_instance_id);
        strcat(fifo_ctr_name, "hda");

		const size_t max_fifo_queue_size = 4*65536;
		//Init thread shared fifos
		fifo_control_direction = fifo_open("fifo_opc_command", max_fifo_queue_size, iec_call_exit_handler);
	};

	~Opc_client_hda_Instance()
	{    
		IT_IT("Opc_client_hda_Instance::~Opc_client_hda_Instance");

		if(Values)
		{
			delete[] Values;
			Values = NULL;
		}
	};
	//
	void Fail(const QString &s)
	{
		FailUnit(s);
		fFail = true;
	};

	InstanceCfg Cfg; // the cacheable stuff
	Driver* ParentDriver;
	QString unit_name;
	
	void driverEvent(DriverEvent *); // message from thread to parent
	bool event(QEvent *e);
	bool Connect();					//connect to the DriverThread
	bool Disconnect();              //disconnect from the DriverThread
	bool DoExec(SendRecePacket *t);
	bool expect(unsigned int cmd);
	void removeTransaction();


	//
	public slots:
	//
	virtual void Start(); // start everything under this driver's control
	virtual void Stop(); // stop everything under this driver's control
	virtual void Command(const QString & name, BYTE cmd, LPVOID lpPa, DWORD pa_length, DWORD ipindex); // process a command for a named unit 
	virtual void QueryResponse (QObject *, const QString &, int, QObject*); // handles database responses
	virtual void Tick();
	//
};

#endif