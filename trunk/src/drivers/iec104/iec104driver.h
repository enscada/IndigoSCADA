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
*Header For:IEC 104
*
*Purpose:
*/

#ifndef include_iec104driver_h 
#define include_iec104driver_h

#include "driver.h"
#include "iec104driverConfiguration.h"
#include "iec104driverInput.h"
#include "iec104driverCommand.h"
#include "sptypes.h"
#include "smplstat.h"
#include "common.h"

#ifdef IEC_104_DRIVER_EXPORTS
#define IEC_104_DRIVERDRV __declspec(dllexport)
#else
#define IEC_104_DRIVERDRV //__declspec(dllimport)
#endif

class Iec104driver_Instance;

struct InstanceCfg // friend to anyone
{
	QString OpcServerProgID;  // opc server ProgID
	QString IEC104ServerIPAddress;  // IEC 104 server IP address (slave)

	unsigned SampleTime; // sampleing time 

	InstanceCfg() : 
	OpcServerProgID(""),SampleTime(1000),IEC104ServerIPAddress("")
	{
	};

	InstanceCfg(const InstanceCfg &m) : 

	OpcServerProgID(m.OpcServerProgID),SampleTime(m.SampleTime),IEC104ServerIPAddress(m.IEC104ServerIPAddress)
	{
	};
};


class IEC_104_DRIVERDRV Iec104driver : public Driver // all this is private to this one file
{
	Q_OBJECT
	public:
	typedef std::map<QString,Iec104driver_Instance *, std::less<QString> > IDict;
	IDict Instances;
	enum
	{
		tListUnits = 1, tcreateNewUnit
	};


	Iec104driver(QObject *parent,const QString &name);
	~Iec104driver();
	//
	// user interface stuff
	// 
	void UnitConfigure(QWidget *parent, const QString &name, const QString &receipe="(default)"); // configure a unit
	void SetTypeList(QComboBox *pCombo, const QString &unitname); // set the type list for unit type
	void GetInputList(const QString &type, QStringList &,const QString &unit, const QString &name); // set the permitted input IDs
	QWidget * GetSpecificConfig(QWidget *, const QString &spname, const QString &sptype); //specific config for sample point of type
	void GetTagList(const QString &type, QStringList &,const QString &,const QString &); // returns the permitted tags for a given type for this unit
	void CreateNewUnit(QWidget *parent, const QString &, int); // create a new unit - quick configure
	void CommandDlg(QWidget *parent, const QString &name); // command dialog

	//
	// 
	static Iec104driver *pDriver; // only one instance should be created

	int n_opc_items;
	QString iec_unit_name;
	//
	// Actual driver stuff
	//
	// these are slots - the declarations are virtual in Driver
	//  
	public slots:
	void Start(); // start everything under this driver's control
	void Stop(); // stop everything under this driver's control
	void Command(const QString &, BYTE, LPVOID, DWORD, DWORD); // process a command for a named unit 
	void QueryResponse (QObject *, const QString &, int, QObject*);
};
#endif

