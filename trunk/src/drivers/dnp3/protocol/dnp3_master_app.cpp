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
#include "iec_item_type.h" //Middleware
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <winsock2.h>
#include "station.hpp"
#include "master.hpp"
#include "datalink.hpp"
#include "custom.hpp"
#include "clear_crc_eight.h"
#include "iec104types.h"
#include "iec_item.h"
#include "process.h"
#include "dnp3_master_app.h"

extern int gl_timeout_connection_with_parent;

////////////////////////////Middleware///////////////////////////////////////////////////////
Boolean  quite = ORTE_FALSE;
int	regfail=0;

//event system
void onRegFail(void *param) 
{
  printf("registration to a manager failed\n");
  regfail = 1;
}

void rebuild_iec_item_message(struct iec_item *item2, iec_item_type *item1)
{
	unsigned char checksum;

	///////////////Rebuild struct iec_item//////////////////////////////////
	item2->iec_type = item1->iec_type;
	memcpy(&(item2->iec_obj), &(item1->iec_obj), sizeof(struct iec_object));
	item2->cause = item1->cause;
	item2->msg_id = item1->msg_id;
	item2->ioa_control_center = item1->ioa_control_center;
	item2->casdu = item1->casdu;
	item2->is_neg = item1->is_neg;
	item2->checksum = item1->checksum;
	///////and check the 1 byte checksum////////////////////////////////////
	checksum = clearCrc((unsigned char *)item2, sizeof(struct iec_item));

	fprintf(stderr,"new checksum = %u\n", checksum);

	//if checksum is 0 then there are no errors
	if(checksum != 0)
	{
		//log error message
		ExitProcess(0);
	}

	fprintf(stderr,"iec_type = %u\n", item2->iec_type);
	fprintf(stderr,"iec_obj = %x\n", item2->iec_obj);
	fprintf(stderr,"cause = %u\n", item2->cause);
	fprintf(stderr,"msg_id =%u\n", item2->msg_id);
	fprintf(stderr,"ioa_control_center = %u\n", item2->ioa_control_center);
	fprintf(stderr,"casdu =%u\n", item2->casdu);
	fprintf(stderr,"is_neg = %u\n", item2->is_neg);
	fprintf(stderr,"checksum = %u\n", item2->checksum);
}

void recvCallBack(const ORTERecvInfo *info,void *vinstance, void *recvCallBackParam) 
{
	DNP3MasterApp * cl = (DNP3MasterApp*)recvCallBackParam;
	iec_item_type *item1 = (iec_item_type*)vinstance;

	switch (info->status) 
	{
		case NEW_DATA:
		{
		  if(!quite)
		  {
			  struct iec_item item2;
			  rebuild_iec_item_message(&item2, item1);
			  cl->received_command_callback = 1;
			  cl->check_for_commands(&item2);
			  cl->received_command_callback = 0;
		  }
		}
		break;
		case DEADLINE:
		{
			printf("deadline occurred\n");
		}
		break;
	}
}
////////////////////////////////Middleware/////////////////////////////////////

////////////////////////////////Middleware/////////////////
iec_item_type DNP3MasterApp::instanceSend;
ORTEPublication* DNP3MasterApp::publisher = NULL;
////////////////////////////////Middleware/////////////////
//   
//  Class constructor.   
//   
DNP3MasterApp::DNP3MasterApp(char* dnp3server_address, char*dnp3server_port, char* line_number, int polling_time)
{   
    this->Connected = false;
	debugLevel = 1;
	integrityPollInterval = 10;
	masterConfig.addr = 1;
	masterConfig.consecutiveTimeoutsForCommsFail = 3;
	masterConfig.integrityPollInterval_p = &integrityPollInterval;
	masterConfig.debugLevel_p = &debugLevel;

	stationConfig.addr = 1;
	stationConfig.debugLevel_p = &debugLevel;

	datalinkConfig.addr                  = masterConfig.addr;
	datalinkConfig.isMaster              = 1;
	datalinkConfig.keepAliveInterval_ms  = 10000;

	tx_var = new CustomInter(&debugLevel, 'M', 'S', getSocket());

	datalinkConfig.tx_p                  = tx_var;
	datalinkConfig.debugLevel_p          = &debugLevel;

	master_p = new Master(masterConfig, datalinkConfig, &stationConfig, 1, &db, &timer);

	fExit = false;

	strcpy(dnp3ServerAddress, dnp3server_address);
	strcpy(dnp3ServerPort, dnp3server_port);

	pollingTime = polling_time;

	/////////////////////Middleware/////////////////////////////////////////////////////////////////
	received_command_callback = 0;

	int32_t                 strength = 1;
	NtpTime                 persistence, deadline, minimumSeparation, delay;
	IPAddress				smIPAddress = IPADDRESS_INVALID;
	
	subscriber = NULL;

	ORTEInit();
	ORTEDomainPropDefaultGet(&dp);
	NTPTIME_BUILD(minimumSeparation,0); 
	NTPTIME_BUILD(delay,1); //1s

	//initiate event system
	ORTEDomainInitEvents(&events);

	events.onRegFail = onRegFail;

	//Create application     
	domain = ORTEDomainAppCreate(ORTE_DEFAULT_DOMAIN,&dp,&events,ORTE_FALSE);

	iec_item_type_type_register(domain);

	//Create publisher
	NTPTIME_BUILD(persistence,5);

	char fifo_monitor_name[150];
	strcpy(fifo_monitor_name,"fifo_monitor_direction");
	strcat(fifo_monitor_name, line_number);
	strcat(fifo_monitor_name, "da");

	publisher = ORTEPublicationCreate(
	domain,
	fifo_monitor_name,
	"iec_item_type",
	&instanceSend,
	&persistence,
	strength,
	NULL,
	NULL,
	NULL);

	//if(publisher == NULL){} //check this error
	
	char fifo_control_name[150];
	strcpy(fifo_control_name,"fifo_control_direction");
	strcat(fifo_control_name, line_number);
	strcat(fifo_control_name, "da");

	//Create subscriber
	NTPTIME_BUILD(deadline,3);

	subscriber = ORTESubscriptionCreate(
	domain,
	IMMEDIATE,
	BEST_EFFORTS,
	fifo_control_name,
	"iec_item_type",
	&instanceRecv,
	&deadline,
	&minimumSeparation,
	recvCallBack,
	this,
	smIPAddress);
	///////////////////////////////////Middleware//////////////////////////////////////////////////

    return;   
}   
//   
//  Class destructor.   
//   
DNP3MasterApp::~DNP3MasterApp()   
{   
    // free resources   
	if(tx_var)
	{
		delete tx_var;
	}

	if(master_p)
	{
		delete master_p;
	}

    CloseLink();
	
    return;   
}   

bool DNP3MasterApp::GetSockConnectStatus(void)
{
	return Connected;
}

//   
//  Open TCP/IP connection.   
//   
int DNP3MasterApp::OpenLink(char *serverIP, int port)   
{   
    WORD wVersionRequested;   
    int wsaerr;   
   
    // connected ?   
    if(GetSockConnectStatus())   
    {   
		// report warning   
		sprintf(LastError, "Socket is connected!\n");   
		fprintf(stderr, "%s\n", LastError);
		fflush(stderr);

		return EIO;   
    }   
    else   
    {   
		// Using MAKEWORD macro, Winsock version request 2.2   
		wVersionRequested = MAKEWORD(2, 2);   
		wsaerr = WSAStartup(wVersionRequested, &wsaData);   
		if(wsaerr != 0)   
		{   
			/* Tell the user that we could not find a usable */   
			/* WinSock DLL.*/   
			sprintf(LastError, "The Winsock dll not found!\n");
			fprintf(stderr, "%s\n", LastError);
			fflush(stderr);

			return ENOENT;   
		}   
	#ifdef _DEBUG   
		sprintf(LastError, "The Winsock dll found! - The status: %s.\n", wsaData.szSystemStatus);
		fprintf(stderr, "%s\n", LastError);
		fflush(stderr);
    
	#endif   
		sprintf(this->RemoteHost, serverIP);

		this->RemotePort = port;
		
		fprintf(stderr, "remote port %d\n", this->RemotePort);
		fflush(stderr);

		// init socket   
		// Create the socket as an IPv4 (AF_INET)
		Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   
		if(Socket == INVALID_SOCKET)   
		{   
			//Output the error recieved when creating the socket.   
			sprintf(LastError, "Socket error: %ld\n", WSAGetLastError());
			fprintf(stderr, "%s\n", LastError);
			fflush(stderr);
        
			WSACleanup();   
			Socket = 0;   
        
			return EIO;   
		}   

		//Assign the remote info to our socket address structure.   
		RemoteInfo.sin_family = AF_INET;   
		RemoteInfo.sin_addr.s_addr = inet_addr(this->RemoteHost);   
		RemoteInfo.sin_port = htons((SHORT)this->RemotePort);   
       
		//Time to connect to the remote computer   
		if(connect(Socket, (SOCKADDR*)&RemoteInfo, sizeof(RemoteInfo)) == SOCKET_ERROR)   
		{   
			sprintf(LastError, "Socket error: Unable to establish a connection to %s:%i!\n", RemoteHost, RemotePort);
			fprintf(stderr, "%s\n", LastError);
			fflush(stderr);
        
			closesocket(Socket);   
			WSACleanup();   
			Socket = 0;   
			return EIO;   
		}   
    }
	
    this->Connected=true;   
    return EOK;   
}   
   
//   
//  Close TCP/IP connection   
//   
int DNP3MasterApp::CloseLink(bool free)   
{   
    Connected = false;   
    shutdown(Socket, SD_BOTH);   
    closesocket(Socket);   
    WSACleanup();   
    return EOK;   
}

void DNP3MasterApp::check_for_commands(struct iec_item *queued_item)
{
	if(!fExit)
	{ 
		fprintf(stderr,"Receiving %d th message \n", queued_item->msg_id);
		fflush(stderr);
					
		/////////////////////write command///////////////////////////////////////////////////////////
		if(queued_item->iec_type == C_SC_TA_1
			|| queued_item->iec_type == C_DC_TA_1
			|| queued_item->iec_type == C_SE_TA_1
			|| queued_item->iec_type == C_SE_TB_1
			|| queued_item->iec_type == C_SE_TC_1
			|| queued_item->iec_type == C_BO_TA_1
			|| queued_item->iec_type == C_SC_NA_1
			|| queued_item->iec_type == C_DC_NA_1
			|| queued_item->iec_type == C_SE_NA_1 
			|| queued_item->iec_type == C_SE_NB_1
			|| queued_item->iec_type == C_SE_NC_1
			|| queued_item->iec_type == C_BO_NA_1)
		{
			Sleep(100); //Delay between one command and the next one

			/////////Here we make the QUERY:////////////////////////////////////////// /////////////////////////////
			//select from Item table hClient where ioa is equal to ioa of packet arriving (command) from monitor.exe
			///////////////////////////////////////////////////////////////////////////////////////
			int found = 0;
			DWORD hClient = -1;
/*
			for(dw = 0; dw < g_dwNumItems; dw++) 
			{ 
				if(queued_item->iec_obj.ioa == Item[dw].ioa_control_center)
				{
					found = 1;
					hClient = Item[dw].hClient;
					break;
				}
			}

			if(found == 0)
			{
				fprintf(stderr,"Error: Command with IOA %d not found in I/O list\n", queued_item->iec_obj.ioa);
				fflush(stderr);
				fprintf(stderr,"Command NOT executed\n");
				fflush(stderr);
				return;
			}
			/////////////////////////////////////////////////////////////////////

			//check iec type of command
			if(Item[hClient - 1].io_list_iec_type != queued_item->iec_type)
			{
				//error
				fprintf(stderr,"Error: Command with IOA %d has iec_type %d, different from IO list type %d\n", queued_item->iec_obj.ioa, queued_item->iec_type, Item[hClient - 1].io_list_iec_type);
				fflush(stderr);
				fprintf(stderr,"Command NOT executed\n");
				fflush(stderr);
				return;
			}
*/
			//Receive a write command
								
			fprintf(stderr,"Receiving command for hClient %d, ioa %d\n", hClient, queued_item->iec_obj.ioa);
			fflush(stderr);
			
			//TODO: implement OPC AE write operation
			
		}
		else if(queued_item->iec_type == C_EX_IT_1)
		{
			//Receiving EXIT process command from monitor.exe
			//exit the thread, and stop the process
			fExit = true;
		}
		else if(queued_item->iec_type == C_IC_NA_1)
		{
			//Do General Interrogation
		}
	}

	return;
}

void DNP3MasterApp::alloc_command_resources(void)
{

}

void DNP3MasterApp::free_command_resources(void)
{

}

int DNP3MasterApp::run(void)
{
	if(!OpenLink(dnp3ServerAddress, atoi(dnp3ServerPort)))
	{  
		//Write
		master_p->poll(Master::INTEGRITY);

		//Read
		char data_p[80];
		int n_read;

		n_read = tx_var->read(getSocket(), data_p, 1, 80, 15);

		if(n_read > 0)
		{
			// put the char data into a Bytes container
			Bytes bytes((unsigned char*)data_p, (unsigned char*)data_p + n_read);

			master_p->rxData(&bytes, 0);
		}
		else	
		{
			fExit = true;
			return 1;
		}

		for(;;)   
		{   
			master_p->startNewTransaction();

			n_read = tx_var->read(getSocket(), data_p, 1, 80, 15);

			if(n_read > 0)
			{
				// put the char data into a Bytes container
				Bytes bytes((unsigned char*)data_p, (unsigned char*)data_p + n_read);

				master_p->rxData(&bytes, 0);
			}
			else
			{
				break; //exit loop
			}

			gl_timeout_connection_with_parent++;

			if(gl_timeout_connection_with_parent > 1000*20/pollingTime)
			{
				break; //exit loop for timeout of connection with parent
			}

			Sleep((unsigned long)pollingTime);
		}   


		Sleep(1000);   
		//CloseLink();   
		fExit = true;
	}   
	else   
	{   
		bool t = GetSockConnectStatus();   
		Sleep(30000);   
		//CloseLink();
		fExit = true;
	}

	return 0;
}
