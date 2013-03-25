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
#define ENOENT 1
#define EIO 1
#define EOK 0

#include "iec_item_type.h" //Middleware
#include "station.hpp"
#include "master.hpp"
#include "datalink.hpp"
#include "custom.hpp"
 
class DNP3MasterApp { 
private: 
	SOCKET      Socket; 
	WSADATA     wsaData; 
	SOCKADDR_IN	RemoteInfo; 
	char        RemoteHost[32]; 
	int         RemotePort; 
	bool		Connected; 
 
	char		ErrorMode; 
	char		LastError[100];
	char		dnp3ServerAddress[80];
	char		dnp3ServerPort[80];
	double		pollingTime;

public: 

	Master* master_p;
	CustomDb db;
	CustomTimer timer;
	int debugLevel;
	int integrityPollInterval;
	Master::MasterConfig          masterConfig;
	Datalink::DatalinkConfig      datalinkConfig;
	Station::StationConfig        stationConfig;
	CustomInter* tx_var;
	/////////////////////Middleware/////////////////////////
	int received_command_callback;
	ORTEDomain              *domain;
	ORTEDomainProp          dp; 
	static ORTEPublication  *publisher;
	ORTESubscription        *subscriber;
	static iec_item_type    instanceSend;
	iec_item_type		    instanceRecv;
	ORTEDomainAppEvents     events;
	///////////////////////////////////Middleware///////////
	bool fExit;
public: 
	DNP3MasterApp(char* dnp3server_address, 
		char*dnp3server_port, 
		char* line_number, 
		int polling_time,
		int nIOA_AO,
		int nIOA_BO,
		int nIOA_CI,
		int nIOA_BI,
		int nIOA_AI);

	~DNP3MasterApp(void); 
	 
	int OpenLink(char *serverIP,int port = 20000); 
	int CloseLink(bool free=true); 
	SOCKET getSocket(void) { return Socket;};
	bool GetSockConnectStatus(void);
	int run(void);
	////////////////////Middleware//////////////////////
	void check_for_commands(struct iec_item *item);
	void alloc_command_resources(void);
	void free_command_resources(void);
	////////////////////Middleware//////////////////////
};

