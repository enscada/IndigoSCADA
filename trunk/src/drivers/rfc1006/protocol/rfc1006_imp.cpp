/*
 *                         IndigoSCADA
 *
 *   This software and documentation are Copyright 2002 to 2012 Enscada 
 *   Limited and its licensees. All rights reserved. See file:
 *
 *                     $HOME/LICENSE 
 *
 *   for full copyright notice and license terms. 
 *
 */

#include "stdint.h"
#include "iec104types.h"
#include "iec_item.h"
#include "clear_crc_eight.h"
#include "rfc1006_item.h"
#include "rfc1006_imp.h"
#include "stdlib.h"

#define MAX_KEYLEN 256
#define MAX_COMMAND_SEND_TIME 60

static int g_dwSleepInLoop = 1000;

#include <sys/stat.h>

extern int gl_timeout_connection_with_parent;

/////////////////////////////////////Middleware///////////////////////////////////////////
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

//	fprintf(stderr,"new checksum = %u\n", checksum);

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
	rfc1006_imp * cl = (rfc1006_imp*)recvCallBackParam;
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
iec_item_type rfc1006_imp::instanceSend;
ORTEPublication* rfc1006_imp::publisher = NULL;
////////////////////////////////Middleware/////////////////

//Global to remove ASAP///////////////////////
iec_item_type* global_instanceSend;
ORTEPublication* global_publisher;
//////////////////////////////////////////////

//   
//  Class constructor.   
//   
rfc1006_imp::rfc1006_imp(char* rfc1006server_address, char*rfc1006server_port, char* line_number, int polling_time):
fExit(false),pollingTime(polling_time)
{   
	strcpy(ServerIPAddress, rfc1006server_address);
	strcpy(ServerPort, rfc1006server_port);

	//if(!OpenLink(ServerIPAddress, atoi(rfc1006ServerPort)))
	{
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
		strcat(fifo_monitor_name, "rfc1006");

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
		strcat(fifo_control_name, "rfc1006");

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

		//if(subscriber == NULL){} //check this error
		///////////////////////////////////Middleware//////////////////////////////////////////////////
		
		////////////remove ASAP/////////////////////////
		//Pass instanceSend and publisher as parameters to Factory calls through Master class
		global_instanceSend = &instanceSend;
		global_publisher = publisher;
		////////////////////////////////////////////////

		//master_p = new Master(masterConfig, datalinkConfig, &stationConfig, 1, &db, &timer);
	}
}   
//   
//  Class destructor.   
//   
rfc1006_imp::~rfc1006_imp()  
{   
    // free resources   
	
    return;   
}   


static u_int n_msg_sent = 0;

int rfc1006_imp::Async2Update()
{
	IT_IT("rfc1006_imp::Async2Update");

	
	int rc = 0;
	int check_server = 0;

	while(true)
	{
		//check connection every g_dwUpdateRate*10 (about 10 or 30 secondi)
		if((check_server%10) == 0)
		{
			rc = check_connection_to_server();
			fprintf(stderr,"check for server connection...\n");
			fflush(stderr);
		}

		check_server++;

		if(rc)
		{ 
			//fprintf(stderr,"rfc1006_imp exiting...., due to lack of connection with server\n");
			//fflush(stderr);
			IT_COMMENT("rfc1006_imp exiting...., due to lack of connection with server");
			
			//Send LOST message to parent (monitor.exe)
			struct iec_item item_to_send;
			struct cp56time2a actual_time;
			get_utc_host_time(&actual_time);

			memset(&item_to_send,0x00, sizeof(struct iec_item));

			item_to_send.iec_obj.ioa = 0;

			item_to_send.cause = 0x03;
			item_to_send.iec_type = C_LO_ST_1;
			item_to_send.iec_obj.o.type30.sp = 0;
			item_to_send.iec_obj.o.type30.time = actual_time;
			item_to_send.iec_obj.o.type30.iv = 0;
			item_to_send.msg_id = n_msg_sent;
			item_to_send.checksum = clearCrc((unsigned char *)&item_to_send, sizeof(struct iec_item));

			//Send in monitor direction
			//prepare published data
			memset(&instanceSend,0x00, sizeof(iec_item_type));

			instanceSend.iec_type = item_to_send.iec_type;
			memcpy(&(instanceSend.iec_obj), &(item_to_send.iec_obj), sizeof(struct iec_object));
			instanceSend.cause = item_to_send.cause;
			instanceSend.msg_id = item_to_send.msg_id;
			instanceSend.ioa_control_center = item_to_send.ioa_control_center;
			instanceSend.casdu = item_to_send.casdu;
			instanceSend.is_neg = item_to_send.is_neg;
			instanceSend.checksum = item_to_send.checksum;

			ORTEPublicationSend(publisher);
		
			break; 
		}

		if(fExit)
		{
			IT_COMMENT("Terminate rfc1006 loop!");
			break;
		}

		gl_timeout_connection_with_parent++;

		if(gl_timeout_connection_with_parent > 1000*20/(int)g_dwSleepInLoop)
		{
			break; //exit loop for timeout of connection with parent
		}
				
		::Sleep(g_dwSleepInLoop);
	}
	
	IT_EXIT;
	return 0;
}

int rfc1006_imp::RfcStart(char* RfcServerProgID, char* RfcclassId, char* RfcUpdateRate, char* RfcPercentDeadband)
{
	IT_IT("rfc1006_imp::RfcStart");
	
	char show_msg[200];
	sprintf(show_msg, " IndigoSCADA RFC1006 Client Start\n");
	LogMessage(NULL, show_msg);

	float fTemp = 0.0f;

	if(strlen(RfcUpdateRate) > 0)
	{
		g_dwUpdateRate = atoi(RfcUpdateRate);
	}

	if(strlen(RfcPercentDeadband) > 0)
	{
		dead_band_percent = atof(RfcPercentDeadband);
	}
	
	if((strlen(ServerIPAddress) == 0) || (strcmp("127.0.0.1", ServerIPAddress) == 0))
	{
		local_server = 1;
	}

	strcpy(plc_server_prog_id, RfcServerProgID);
	

	IT_EXIT;
    return(0);
}

int rfc1006_imp::RfcStop()
{
	IT_IT("rfc1006_imp::RfcStop");

	fprintf(stderr,"Entering RfcStop()\n");
	fflush(stderr);

	while(received_command_callback)
	{
		Sleep(100);
	}
	
	// terminate server and it will clean up itself

	char show_msg[200];
	sprintf(show_msg, " IndigoSCADA RFC1006 Client End\n");
	LogMessage(NULL, show_msg);

	IT_EXIT;
	return 1;
}

int rfc1006_imp::check_connection_to_server(void)
{
	IT_IT("rfc1006_imp::check_connection_to_server");

	WORD wMajor, wMinor, wBuild;

	LPWSTR pwsz = NULL;

	if(!GetStatus(&wMajor, &wMinor, &wBuild, &pwsz))
	{
		
	}
	else
	{
		IT_EXIT;
		return 1;
	}

	return 0;
}

int rfc1006_imp::GetStatus(WORD *pwMav, WORD *pwMiv, WORD *pwB, LPWSTR *pszV)
{
	IT_IT("rfc1006_imp::GetStatus");
	
	IT_EXIT;
	return 0;
}

struct log_message{

	int ioa;
	char message[150];
};

void rfc1006_imp::LogMessage(int* error, const char* name)
{
	//TODO: send message to monitor.exe as a single point

	/*
	struct iec_item item_to_send;
	struct cp56time2a actual_time;
	get_utc_host_time(&actual_time);

	memset(&item_to_send,0x00, sizeof(struct iec_item));

	//item_to_send.iec_obj.ioa =  Find ioa given the message in a vector of log_message

	item_to_send.cause = 0x03;
	item_to_send.iec_type = M_SP_TB_1;
	item_to_send.iec_obj.o.type30.sp = 0;
	item_to_send.iec_obj.o.type30.time = actual_time;
	item_to_send.iec_obj.o.type30.iv = 0;
	*/
}

#include <time.h>
#include <sys/timeb.h>

void rfc1006_imp::get_utc_host_time(struct cp56time2a* time)
{
	struct timeb tb;
	struct tm	*ptm;
		
	IT_IT("get_utc_host_time");

    ftime (&tb);
	ptm = gmtime(&tb.time);
		
	time->hour = ptm->tm_hour;					//<0..23>
	time->min = ptm->tm_min;					//<0..59>
	time->msec = ptm->tm_sec*1000 + tb.millitm; //<0..59999>
	time->mday = ptm->tm_mday; //<1..31>
	time->wday = (ptm->tm_wday == 0) ? ptm->tm_wday + 7 : ptm->tm_wday; //<1..7>
	time->month = ptm->tm_mon + 1; //<1..12>
	time->year = ptm->tm_year - 100; //<0.99>
	time->iv = 0; //<0..1> Invalid: <0> is valid, <1> is invalid
	time->su = (u_char)tb.dstflag; //<0..1> SUmmer time: <0> is standard time, <1> is summer time

	IT_EXIT;
    return;
}

time_t rfc1006_imp::epoch_from_cp56time2a(const struct cp56time2a* time)
{
	struct tm	t;
	time_t epoch = 0;
	int ms;
	
	memset(&t, 0x00, sizeof(struct tm));
	
	t.tm_hour = time->hour;
	t.tm_min = time->min;
	t.tm_sec = time->msec/1000;
	ms = time->msec%1000; //not used
	t.tm_mday = time->mday;
	t.tm_mon = time->month - 1;	  //from <1..12> to	<0..11>				
	t.tm_year = time->year + 100; //from <0..99> to <years from 1900>
	t.tm_isdst = time->su;
	
	epoch = mktime(&t);

	if((epoch == -1) || (time->iv == 1))
	{
		epoch = 0;
	}

	return epoch;
}

void rfc1006_imp::epoch_to_cp56time2a(cp56time2a *time, signed __int64 epoch_in_millisec)
{
	struct tm	*ptm;
	int ms = (int)(epoch_in_millisec%1000);
	time_t seconds;

	IT_IT("epoch_to_cp56time2a");
	
	memset(time, 0x00,sizeof(cp56time2a));
	seconds = (long)(epoch_in_millisec/1000);
	ptm = localtime(&seconds);
		
    if(ptm)
	{
		time->hour = ptm->tm_hour;					//<0.23>
		time->min = ptm->tm_min;					//<0..59>
		time->msec = ptm->tm_sec*1000 + ms; //<0.. 59999>
		time->mday = ptm->tm_mday; //<1..31>
		time->wday = (ptm->tm_wday == 0) ? ptm->tm_wday + 7 : ptm->tm_wday; //<1..7>
		time->month = ptm->tm_mon + 1; //<1..12>
		time->year = ptm->tm_year - 100; //<0.99>
		time->iv = 0; //<0..1> Invalid: <0> is valid, <1> is invalid
		time->su = (u_char)ptm->tm_isdst; //<0..1> SUmmer time: <0> is standard time, <1> is summer time
	}

	IT_EXIT;
    return;
}

//#define ABS(x) ((x) >= 0 ? (x) : -(x))

void rfc1006_imp::SendEvent2(void)
{
	IT_IT("rfc1006_imp::SendEvent2");
/*
	VARIANT *pValue;
	const FILETIME* ft;
	DWORD pwQualities;
	OPCHANDLE phClientItem;
	unsigned char cot;
	cp56time2a time;
	signed __int64 epoch_in_millisec;
	struct iec_item item_to_send;
	double delta = 0.0;
	
	USES_CONVERSION;

	IT_COMMENT1("pwQualities = %d", pwQualities);
	IT_COMMENT1("phClientItem = %d", phClientItem);

	if(Item == NULL)
	{
		//print error message
		return;
	}
    	
	memset(&item_to_send,0x00, sizeof(struct iec_item));
		
	item_to_send.iec_obj.ioa = Item[phClientItem - 1].ioa_control_center;

	item_to_send.cause = cot;

	const char* parc = (const char*)W2T(Item[phClientItem - 1].wszName); // togliere - 1 poiche il primo elemento del vettore Item parte da 0
	
	if(parc == NULL)
	{
		//print error message
		return;
	}
		
	//strcpy(item_to_send.opc_server_item_id, parc);

	epoch_in_millisec = epoch_from_FILETIME(ft);
	
	if(!pValue)
	{
		VARIANT Value;
		pValue = &Value;
		V_VT(pValue) = VT_EMPTY;
	}
	
	switch(V_VT(pValue))
	{
		//case VT_EMPTY:
		//{
			//IT_COMMENT1("Value = %d", V_EMPTY(pValue));
		//}
		//break;
		case VT_I1:
		{
			switch(Item[phClientItem - 1].io_list_iec_type)
			{
				case M_IT_TB_1:
				{
					item_to_send.iec_type = M_IT_TB_1;
					epoch_to_cp56time2a(&time, epoch_in_millisec);
					item_to_send.iec_obj.o.type37.counter = V_I1(pValue);
					item_to_send.iec_obj.o.type37.time = time;
						
					if(pwQualities != OPC_QUALITY_GOOD)
						item_to_send.iec_obj.o.type37.iv = 1;

					IT_COMMENT1("Value = %d", V_I1(pValue));
				}
				break;
				case M_ME_TE_1:
				{
					int error = 0;

					item_to_send.iec_type = M_ME_TE_1;
					epoch_to_cp56time2a(&time, epoch_in_millisec);
					item_to_send.iec_obj.o.type35.mv = rescale_value(V_I1(pValue),
					Item[phClientItem - 1].min_measure, 
					Item[phClientItem - 1].max_measure, &error);
					
					if(pwQualities != OPC_QUALITY_GOOD)
						item_to_send.iec_obj.o.type35.iv = 1;

					IT_COMMENT1("Value = %d", V_I1(pValue));
				}
				break;
				case C_DC_NA_1:
				{
					fprintf(stderr,"IEC type %d is NOT sent in monitoring direction\n", Item[phClientItem - 1].io_list_iec_type);
					fflush(stderr);
				}
				break;
				default:
				{
				  fprintf(stderr,"IEC type %d is NOT supported for VT_I1\n", Item[phClientItem - 1].io_list_iec_type);
				  fflush(stderr);
				}
				break;
			}
		}
		break;
		case VT_UI1:
		{
			switch(Item[phClientItem - 1].io_list_iec_type)
			{
				case M_IT_TB_1:
				{
					item_to_send.iec_type = M_IT_TB_1;
					epoch_to_cp56time2a(&time, epoch_in_millisec);
					item_to_send.iec_obj.o.type37.counter = V_UI1(pValue);
					item_to_send.iec_obj.o.type37.time = time;
						
					if(pwQualities != OPC_QUALITY_GOOD)
						item_to_send.iec_obj.o.type37.iv = 1;

					IT_COMMENT1("Value = %d", V_UI1(pValue));
				}
				break;
				case M_ME_TE_1:
				{
					int error = 0;

					item_to_send.iec_type = M_ME_TE_1;
					epoch_to_cp56time2a(&time, epoch_in_millisec);
					item_to_send.iec_obj.o.type35.mv = rescale_value(V_UI1(pValue),
					Item[phClientItem - 1].min_measure, 
					Item[phClientItem - 1].max_measure, &error);
					
					if(pwQualities != OPC_QUALITY_GOOD)
						item_to_send.iec_obj.o.type35.iv = 1;

					IT_COMMENT1("Value = %d", V_UI1(pValue));
				}
				break;
				case C_DC_NA_1:
				{
					fprintf(stderr,"IEC type %d is NOT sent in monitoring direction\n", Item[phClientItem - 1].io_list_iec_type);
					fflush(stderr);
				}
				break;
				default:
				{
				  fprintf(stderr,"IEC type %d is NOT supported for VT_UI1\n", Item[phClientItem - 1].io_list_iec_type);
				  fflush(stderr);
				}
				break;
			}
		}
		break;
		case VT_I2:
		{
				switch(Item[phClientItem - 1].io_list_iec_type)
				{
					case M_IT_TB_1:
					{
						item_to_send.iec_type = M_IT_TB_1;
						epoch_to_cp56time2a(&time, epoch_in_millisec);
						item_to_send.iec_obj.o.type37.counter = V_I2(pValue);
						item_to_send.iec_obj.o.type37.time = time;
							
						if(pwQualities != OPC_QUALITY_GOOD)
							item_to_send.iec_obj.o.type37.iv = 1;

						IT_COMMENT1("Value = %d", V_I2(pValue));
					}
					break;
					case M_ME_TE_1:
					{
						int error = 0;

						item_to_send.iec_type = M_ME_TE_1;
						epoch_to_cp56time2a(&time, epoch_in_millisec);
						item_to_send.iec_obj.o.type35.mv = rescale_value(V_I2(pValue),
						Item[phClientItem - 1].min_measure, 
						Item[phClientItem - 1].max_measure, &error);
						
						if(pwQualities != OPC_QUALITY_GOOD)
							item_to_send.iec_obj.o.type35.iv = 1;

						IT_COMMENT1("Value = %d", V_I2(pValue));
					}
					break;
					case C_DC_NA_1:
					{
						fprintf(stderr,"IEC type %d is NOT sent in monitoring direction\n", Item[phClientItem - 1].io_list_iec_type);
						fflush(stderr);
					}
					break;
					default:
					{
					  fprintf(stderr,"IEC type %d is NOT supported for VT_I2\n", Item[phClientItem - 1].io_list_iec_type);
					  fflush(stderr);
					}
					break;
				}
		}
		break;
		case VT_UI2:
		{
				switch(Item[phClientItem - 1].io_list_iec_type)
				{
					case M_IT_TB_1:
					{
						item_to_send.iec_type = M_IT_TB_1;
						epoch_to_cp56time2a(&time, epoch_in_millisec);
						item_to_send.iec_obj.o.type37.counter = V_UI2(pValue);
						item_to_send.iec_obj.o.type37.time = time;
							
						if(pwQualities != OPC_QUALITY_GOOD)
							item_to_send.iec_obj.o.type37.iv = 1;

						IT_COMMENT1("Value = %d", V_UI2(pValue));
					}
					break;
					case M_ME_TE_1:
					{
						int error = 0;

						item_to_send.iec_type = M_ME_TE_1;
						epoch_to_cp56time2a(&time, epoch_in_millisec);
						item_to_send.iec_obj.o.type35.mv = rescale_value(V_UI2(pValue),
						Item[phClientItem - 1].min_measure, 
						Item[phClientItem - 1].max_measure, &error);
						
						if(pwQualities != OPC_QUALITY_GOOD)
							item_to_send.iec_obj.o.type35.iv = 1;

						IT_COMMENT1("Value = %d", V_UI2(pValue));
					}
					break;
					default:
					{
					  fprintf(stderr,"IEC type %d is NOT supported for VT_UI2\n", Item[phClientItem - 1].io_list_iec_type);
					  fflush(stderr);
					}
					break;
				}
		}
		break;
		case VT_I4:
		{
				switch(Item[phClientItem - 1].io_list_iec_type)
				{
					case M_IT_TB_1:
					{
						item_to_send.iec_type = M_IT_TB_1;
						epoch_to_cp56time2a(&time, epoch_in_millisec);
						item_to_send.iec_obj.o.type37.counter = V_I4(pValue);
						item_to_send.iec_obj.o.type37.time = time;
							
						if(pwQualities != OPC_QUALITY_GOOD)
							item_to_send.iec_obj.o.type37.iv = 1;

						IT_COMMENT1("Value = %d", V_I4(pValue));
					}
					break;
					case M_ME_TE_1:
					{
						int error = 0;

						item_to_send.iec_type = M_ME_TE_1;
						epoch_to_cp56time2a(&time, epoch_in_millisec);
						item_to_send.iec_obj.o.type35.mv = rescale_value(V_I4(pValue),
						Item[phClientItem - 1].min_measure, 
						Item[phClientItem - 1].max_measure, &error);
							
						if(pwQualities != OPC_QUALITY_GOOD)
							item_to_send.iec_obj.o.type35.iv = 1;

						IT_COMMENT1("Value = %d", V_I4(pValue));
					}
					break;
					default:
					{
					  fprintf(stderr,"IEC type %d is NOT supported for VT_I4\n", Item[phClientItem - 1].io_list_iec_type);
					  fflush(stderr);
					}
					break;
				}
		}
		break;
		case VT_UI4:
		{
				switch(Item[phClientItem - 1].io_list_iec_type)
				{
					case M_IT_TB_1:
					{
						item_to_send.iec_type = M_IT_TB_1;
						epoch_to_cp56time2a(&time, epoch_in_millisec);
						item_to_send.iec_obj.o.type37.counter = V_UI4(pValue); //over 2^31 - 1 there is overflow!
						item_to_send.iec_obj.o.type37.time = time;
							
						if(pwQualities != OPC_QUALITY_GOOD)
							item_to_send.iec_obj.o.type37.iv = 1;

						IT_COMMENT1("Value = %d", V_UI4(pValue));
					}
					break;
					case M_ME_TE_1:
					{
						int error = 0;

						item_to_send.iec_type = M_ME_TE_1;
						epoch_to_cp56time2a(&time, epoch_in_millisec);
						item_to_send.iec_obj.o.type35.mv = rescale_value(V_UI4(pValue),
						Item[phClientItem - 1].min_measure, 
						Item[phClientItem - 1].max_measure, &error);
							
						if(pwQualities != OPC_QUALITY_GOOD)
							item_to_send.iec_obj.o.type35.iv = 1;

						IT_COMMENT1("Value = %d", V_UI4(pValue));
					}
					break;
					default:
					{
					  fprintf(stderr,"IEC type %d is NOT supported for VT_UI4\n", Item[phClientItem - 1].io_list_iec_type);
					  fflush(stderr);
					}
					break;
				}
		}
		break;
		case VT_R4:
		{
			item_to_send.iec_type = M_ME_TF_1;
			epoch_to_cp56time2a(&time, epoch_in_millisec);
			item_to_send.iec_obj.o.type36.mv = V_R4(pValue);
			item_to_send.iec_obj.o.type36.time = time;

			if(pwQualities != OPC_QUALITY_GOOD)
				item_to_send.iec_obj.o.type36.iv = 1;

			IT_COMMENT1("Value = %f", V_R4(pValue));
		}
		break;
		case VT_R8:
		{
			item_to_send.iec_type = M_ME_TN_1;
			epoch_to_cp56time2a(&time, epoch_in_millisec);
			item_to_send.iec_obj.o.type150.mv = V_R8(pValue);
			item_to_send.iec_obj.o.type150.time = time;

			if(pwQualities != OPC_QUALITY_GOOD)
				item_to_send.iec_obj.o.type150.iv = 1;
			
			IT_COMMENT1("Value = %lf", V_R8(pValue));
		}
		break;
		case VT_BOOL:
		{
				item_to_send.iec_type = M_SP_TB_1;
				epoch_to_cp56time2a(&time, epoch_in_millisec);
				item_to_send.iec_obj.o.type30.sp = (V_BOOL(pValue) < 0 ? 1 : 0);
				item_to_send.iec_obj.o.type30.time = time;

				if(pwQualities != OPC_QUALITY_GOOD)
					item_to_send.iec_obj.o.type30.iv = 1;
				
				IT_COMMENT1("Value = %d", V_BOOL(pValue));
		}
		break;
		case VT_DATE:
		{
		}
		break;
		case VT_BSTR:
		{
				//fprintf(stderr,"ItemID = %s: ", (const char*)W2T(Item[phClientItem - 1].wszName));
				//fflush(stderr);

				//fprintf(stderr,"%ls\t", V_BSTR(pValue));
				//fflush(stderr);
				//IT_COMMENT1("Value STRING = %ls", V_BSTR(pValue));

				//Definizione di BSTR:
				//typedef OLECHAR *BSTR;

				//Conversioni:

				//Da const char* a OLE

				//TCHAR* pColumnName
				//OLECHAR*    pOleColumnName = T2OLE(pColumnName);
									
				//Da OLE a const char*
				TCHAR* str = OLE2T(pValue->bstrVal);
				//fprintf(stderr,"%s\n", str);
				//fflush(stderr);
				
				item_to_send.iec_type = M_ME_TF_1;
				epoch_to_cp56time2a(&time, epoch_in_millisec);
							
				item_to_send.iec_obj.o.type36.mv = (float)atof(str);
				item_to_send.iec_obj.o.type36.time = time;

				if(pwQualities != OPC_QUALITY_GOOD)
					item_to_send.iec_obj.o.type36.iv = 1;
										
				IT_COMMENT1("Value STRING = %s", str);
		}
		break;
		default:
		{
			IT_COMMENT1("V_VT(pValue) non gestito = %d", V_VT(pValue));
		
			item_to_send.iec_type = 0;
		}
		break;
	}

	//IT_COMMENT6("at time: %d_%d_%d_%d_%d_%d", time.hour, time.min, time.msec, time.mday, time.month, time.year);

	item_to_send.msg_id = n_msg_sent;
	item_to_send.checksum = clearCrc((unsigned char *)&item_to_send, sizeof(struct iec_item));

	//unsigned char buf[sizeof(struct iec_item)];
	//int len = sizeof(struct iec_item);
	//memcpy(buf, &item_to_send, len);
	//	for(j = 0;j < len; j++)
	//	{
	//	  unsigned char c = *(buf + j);
		//fprintf(stderr,"tx ---> 0x%02x\n", c);
		//fflush(stderr);
		//IT_COMMENT1("tx ---> 0x%02x\n", c);
	//	}

	Sleep(10); //Without delay there is missing of messages in the loading

	//Send in monitor direction
	fprintf(stderr,"Sending message %u th\n", n_msg_sent);
	fflush(stderr);
	IT_COMMENT1("Sending message %u th\n", n_msg_sent);

	//prepare published data
	memset(&instanceSend,0x00, sizeof(iec_item_type));

	instanceSend.iec_type = item_to_send.iec_type;
	memcpy(&(instanceSend.iec_obj), &(item_to_send.iec_obj), sizeof(struct iec_object));
	instanceSend.cause = item_to_send.cause;
	instanceSend.msg_id = item_to_send.msg_id;
	instanceSend.ioa_control_center = item_to_send.ioa_control_center;
	instanceSend.casdu = item_to_send.casdu;
	instanceSend.is_neg = item_to_send.is_neg;
	instanceSend.checksum = item_to_send.checksum;

	ORTEPublicationSend(publisher);

	n_msg_sent++;

	IT_EXIT;
*/
}

//The FILETIME structure is a 64-bit value representing the number 
//of 100-nanosecond intervals since January 1, 1601.
//
//epoch_in_millisec is a 64-bit value representing the number of milliseconds 
//elapsed since January 1, 1970

signed __int64 rfc1006_imp::epoch_from_FILETIME(const FILETIME *fileTime)
{
	IT_IT("epoch_from_FILETIME");
	
	FILETIME localTime;
	struct tm	t;

	time_t sec;
	signed __int64 epoch_in_millisec;

	if(fileTime == NULL)
	{
		IT_EXIT;
		return 0;
	}
	
	// first convert file time (UTC time) to local time
	if (!FileTimeToLocalFileTime(fileTime, &localTime))
	{
		IT_EXIT;
		return 0;
	}

	// then convert that time to system time
	SYSTEMTIME sysTime;
	if (!FileTimeToSystemTime(&localTime, &sysTime))
	{
		IT_EXIT;
		return 0;
	}
	
	memset(&t, 0x00, sizeof(struct tm));
	
	t.tm_hour = sysTime.wHour;
	t.tm_min = sysTime.wMinute;
	t.tm_sec = sysTime.wSecond;
	t.tm_mday = sysTime.wDay;
	t.tm_mon = sysTime.wMonth - 1;
	t.tm_year = sysTime.wYear - 1900; //tm_year contains years after 1900
	t.tm_isdst = -1; //to force mktime to check for dst
	
	sec = mktime(&t);

	if(sec < 0)
	{
		IT_EXIT;
		return 0;
	}

	epoch_in_millisec =  (signed __int64)sec;

	epoch_in_millisec =  epoch_in_millisec*1000 + sysTime.wMilliseconds;

	IT_EXIT;
	return epoch_in_millisec;
}

#define _EPSILON_ ((double)(2.220446E-16))

#define DO_NOT_RESCALE

short rfc1006_imp::rescale_value(double V, double Vmin, double Vmax, int* error)
{
	#ifdef DO_SCALE
	double Amin;
	double Amax;
	double r;
	//double V; //Observed value in ingegneristic unit
	double A = 0.0; //Calculate scaled value between Amin = -32768 and Amax = 32767
	double denomin;

	IT_IT("rescale_value");

	*error = 0;

	Amin = -32768.0;
	Amax = 32767.0;

	if(((V - Vmin) > 0.0) && ((V - Vmax) < 0.0))
	{
		denomin = Vmax - Vmin;

		if(denomin > 0.0)
		{
			r = (Amax - Amin)/denomin;
			A = r*(V - Vmin) + Amin;
		}
		else
		{
			*error = 1;
		}
	}
	else if(((V - Vmin) < 0.0))
	{
		A = Amin;
	}
	else if(!fcmp(V, Vmin, _EPSILON_))
	{
		A = Amin;
	}
	else if(((V - Vmax) > 0.0))
	{
		A = Amax;
	}
	else if(!fcmp(V, Vmax, _EPSILON_))
	{
		A = Amax;
	}
	
	IT_COMMENT4("V = %lf, Vmin = %lf, Vmax = %lf, A = %lf", V, Vmin, Vmax, A);

	IT_EXIT;

	return (short)A;

	#endif

	#ifdef DO_NOT_RESCALE

	return (short)V;

	#endif //DO_NOT_RESCALE
}

double rfc1006_imp::rescale_value_inv(double A, double Vmin, double Vmax, int* error)
{
	#ifdef DO_SCALE
	double Amin;
	double Amax;
	double r;
	double V; //Calculated value in ingegneristic unit
	//double A = 0.0; //Given a scaled value between Amin = -32768 and Amax = 32767
	double denomin;

	IT_IT("rescale_value_inv");

	*error = 0;

	Amin = -32768.0;
	Amax = 32767.0;

	denomin = Vmax - Vmin;

	if(denomin > 0.0)
	{
		r = (Amax - Amin)/denomin;
		V = Vmin + (A - Amin)/r;
	}
	else
	{
		*error = 1;
	}
		
	IT_COMMENT4("V = %lf, Vmin = %lf, Vmax = %lf, A = %lf", V, Vmin, Vmax, A);

	IT_EXIT;

	return V;

	#endif

	#ifdef DO_NOT_RESCALE

	return A;

	#endif //DO_NOT_RESCALE
}


void rfc1006_imp::check_for_commands(struct iec_item *queued_item)
{
	/*
	DWORD dw = 0;
	DWORD nWriteItems = ITEM_WRITTEN_AT_A_TIME;
	HRESULT hr = S_OK;
	HRESULT *pErrorsWrite = NULL;
	HRESULT *pErrorsRead = NULL;


	struct cp56time2a actual_time;
	get_utc_host_time(&actual_time);

	time_t command_arrive_time_in_seconds = epoch_from_cp56time2a(&actual_time);

	while(!g_pIOPCAsyncIO2)
	{
		//LogMessage(E_FAIL,"g_pIOPCAsyncIO2 == NULL");
		//fprintf(stderr,"Exit al line %d\n", __LINE__);
		//fflush(stderr);

		Sleep(100);
		
		if(g_pIOPCAsyncIO2)
		{
			break;
		}
		else
		{
			get_utc_host_time(&actual_time);

			time_t attual_time_in_seconds = epoch_from_cp56time2a(&actual_time);

			if(attual_time_in_seconds - command_arrive_time_in_seconds > 10)
			{
				ExitProcess(0);
			}
		}
	}
        
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
			// select from Item table hClient where ioa is equal to ioa of packet arriving (command) from monitor.exe
			///////////////////////////////////////////////////////////////////////////////////////
			int found = 0;
			DWORD hClient = -1;

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
			#ifdef CHECK_TYPE
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
			#endif

			//Receive a write command
								
			fprintf(stderr,"Receiving command for hClient %d, ioa %d\n", hClient, queued_item->iec_obj.ioa);
			fflush(stderr);
			
			//Check the life time of the command/////////////////////////////////////////////////////////////////
			//If life time > MAX_COMMAND_SEND_TIME seconds => DO NOT execute the command

			time_t command_generation_time_in_seconds = 0;

			switch(queued_item->iec_type)
			{
				case C_SC_TA_1:
				case C_SC_NA_1:
				{
					//time contains the UTC time
					command_generation_time_in_seconds = epoch_from_cp56time2a(&(queued_item->iec_obj.o.type58.time));

					fprintf(stderr,"Command generation at UTC time: h:%i m:%i s:%i ms:%i %02i-%02i-%02i, iv %i, su %i\n",
					queued_item->iec_obj.o.type58.time.hour,
					queued_item->iec_obj.o.type58.time.min,
					queued_item->iec_obj.o.type58.time.msec/1000,
					queued_item->iec_obj.o.type58.time.msec%1000,
					queued_item->iec_obj.o.type58.time.mday,
					queued_item->iec_obj.o.type58.time.month,
					queued_item->iec_obj.o.type58.time.year,
					queued_item->iec_obj.o.type58.time.iv,
					queued_item->iec_obj.o.type58.time.su);
					fflush(stderr);
				}
				break;
				case C_DC_TA_1:
				case C_DC_NA_1:
				{
					//time contains the UTC time
					command_generation_time_in_seconds = epoch_from_cp56time2a(&(queued_item->iec_obj.o.type59.time));

					fprintf(stderr,"Command generation at UTC time: h:%i m:%i s:%i ms:%i %02i-%02i-%02i, iv %i, su %i\n",
					queued_item->iec_obj.o.type59.time.hour,
					queued_item->iec_obj.o.type59.time.min,
					queued_item->iec_obj.o.type59.time.msec/1000,
					queued_item->iec_obj.o.type59.time.msec%1000,
					queued_item->iec_obj.o.type59.time.mday,
					queued_item->iec_obj.o.type59.time.month,
					queued_item->iec_obj.o.type59.time.year,
					queued_item->iec_obj.o.type59.time.iv,
					queued_item->iec_obj.o.type59.time.su);
					fflush(stderr);
				}
				break;
				case C_SE_TA_1:
				case C_SE_NA_1:
				{
					//time contains the UTC time
					command_generation_time_in_seconds = epoch_from_cp56time2a(&(queued_item->iec_obj.o.type61.time));

					fprintf(stderr,"Command generation at UTC time: h:%i m:%i s:%i ms:%i %02i-%02i-%02i, iv %i, su %i\n",
					queued_item->iec_obj.o.type61.time.hour,
					queued_item->iec_obj.o.type61.time.min,
					queued_item->iec_obj.o.type61.time.msec/1000,
					queued_item->iec_obj.o.type61.time.msec%1000,
					queued_item->iec_obj.o.type61.time.mday,
					queued_item->iec_obj.o.type61.time.month,
					queued_item->iec_obj.o.type61.time.year,
					queued_item->iec_obj.o.type61.time.iv,
					queued_item->iec_obj.o.type61.time.su);
					fflush(stderr);
				}
				break;
				case C_SE_TB_1:
				case C_SE_NB_1:
				{
					//time contains the UTC time
					command_generation_time_in_seconds = epoch_from_cp56time2a(&(queued_item->iec_obj.o.type62.time));

					fprintf(stderr,"Command generation at UTC time: h:%i m:%i s:%i ms:%i %02i-%02i-%02i, iv %i, su %i\n",
					queued_item->iec_obj.o.type62.time.hour,
					queued_item->iec_obj.o.type62.time.min,
					queued_item->iec_obj.o.type62.time.msec/1000,
					queued_item->iec_obj.o.type62.time.msec%1000,
					queued_item->iec_obj.o.type62.time.mday,
					queued_item->iec_obj.o.type62.time.month,
					queued_item->iec_obj.o.type62.time.year,
					queued_item->iec_obj.o.type62.time.iv,
					queued_item->iec_obj.o.type62.time.su);
					fflush(stderr);
				}
				break;
				case C_SE_TC_1:
				case C_SE_NC_1:
				{
					//time contains the UTC time
					command_generation_time_in_seconds = epoch_from_cp56time2a(&(queued_item->iec_obj.o.type63.time));

					fprintf(stderr,"Command generation at UTC time: h:%i m:%i s:%i ms:%i %02i-%02i-%02i, iv %i, su %i\n",
					queued_item->iec_obj.o.type63.time.hour,
					queued_item->iec_obj.o.type63.time.min,
					queued_item->iec_obj.o.type63.time.msec/1000,
					queued_item->iec_obj.o.type63.time.msec%1000,
					queued_item->iec_obj.o.type63.time.mday,
					queued_item->iec_obj.o.type63.time.month,
					queued_item->iec_obj.o.type63.time.year,
					queued_item->iec_obj.o.type63.time.iv,
					queued_item->iec_obj.o.type63.time.su);
					fflush(stderr);
				}
				break;
				case C_BO_TA_1:
				case C_BO_NA_1:
				{
					//time contains the UTC time
					command_generation_time_in_seconds = epoch_from_cp56time2a(&(queued_item->iec_obj.o.type64.time));

					fprintf(stderr,"Command generation at UTC time: h:%i m:%i s:%i ms:%i %02i-%02i-%02i, iv %i, su %i\n",
					queued_item->iec_obj.o.type64.time.hour,
					queued_item->iec_obj.o.type64.time.min,
					queued_item->iec_obj.o.type64.time.msec/1000,
					queued_item->iec_obj.o.type64.time.msec%1000,
					queued_item->iec_obj.o.type64.time.mday,
					queued_item->iec_obj.o.type64.time.month,
					queued_item->iec_obj.o.type64.time.year,
					queued_item->iec_obj.o.type64.time.iv,
					queued_item->iec_obj.o.type64.time.su);
					fflush(stderr);
				}
				break;
				default:
				{
					//error
					//fprintf(stderr,"Error %d, %s\n",__LINE__, __FILE__);
					//fflush(stderr);

					char show_msg[200];
					sprintf(show_msg, "Error %d, %s\n",__LINE__, __FILE__);
					rfc1006_imp::LogMessage(0, show_msg);
				
					return;
				}
				break;
			}

			struct cp56time2a actual_time;
			get_utc_host_time(&actual_time);

			time_t command_execution_time_in_seconds = epoch_from_cp56time2a(&actual_time);

			fprintf(stderr,"Command execution UTC time: h:%i m:%i s:%i ms:%i %02i-%02i-%02i, iv %i, su %i\n",
			actual_time.hour,
			actual_time.min,
			actual_time.msec/1000,
			actual_time.msec%1000,
			actual_time.mday,
			actual_time.month,
			actual_time.year,
			actual_time.iv,
			actual_time.su);
			fflush(stderr);

			time_t delta = command_execution_time_in_seconds  - command_generation_time_in_seconds;

			fprintf(stderr,"Aged delta time= %d\n", delta);
			fflush(stderr);

			if(delta < MAX_COMMAND_SEND_TIME && delta >= 0)
			{
				hServer[id_of_ItemToWrite] = Item[hClient - 1].hServer; //<--the server handle identifies the item to write

				switch(V_VT(&Item[hClient - 1]))
				{
					case VT_BSTR:
					{
						#define COMMAND_STR_LEN 20
						char command_string[COMMAND_STR_LEN];

						double val_to_write = 0.0;
						
						switch(queued_item->iec_type)
						{
							case C_SC_TA_1:
							{
								val_to_write = queued_item->iec_obj.o.type58.scs;
								sprintf(command_string, "%f", val_to_write);
							}
							break;
							case C_DC_TA_1:
							{
								val_to_write = queued_item->iec_obj.o.type59.dcs;
								sprintf(command_string, "%f", val_to_write);
							}
							break;
							case C_SE_TA_1:
							{
								double Vmin = Item[hClient - 1].min_measure;
								double Vmax = Item[hClient - 1].max_measure;
								double A = (double)queued_item->iec_obj.o.type61.sv;
								int error = 0;

								val_to_write = rescale_value_inv(A, Vmin, Vmax, &error);
								if(error){ return;}

								sprintf(command_string, "%lf", val_to_write);
							}
							break;
							case C_SE_TB_1:
							{
								double Vmin = Item[hClient - 1].min_measure;
								double Vmax = Item[hClient - 1].max_measure;
								double A = (double)queued_item->iec_obj.o.type62.sv;
								int error = 0;

								val_to_write = rescale_value_inv(A, Vmin, Vmax, &error);
								if(error){ return;}
								
								sprintf(command_string, "%lf", val_to_write);
							}
							break;
							case C_SE_TC_1:
							{
								val_to_write = queued_item->iec_obj.o.type63.sv;
								sprintf(command_string, "%lf", val_to_write);
							}
							break;
							case C_BO_TA_1:
							{
								memset(command_string, 0x00, COMMAND_STR_LEN);
								memcpy(command_string, &(queued_item->iec_obj.o.type64.stcd), sizeof(struct iec_stcd));
							}
							break;
							case C_SC_NA_1:
							{
								val_to_write = queued_item->iec_obj.o.type45.scs;
								sprintf(command_string, "%lf", val_to_write);
							}
							break;
							case C_DC_NA_1:
							{
								val_to_write = queued_item->iec_obj.o.type46.dcs;
								sprintf(command_string, "%lf", val_to_write);
							}
							break;
							case C_SE_NA_1:
							{
								double Vmin = Item[hClient - 1].min_measure;
								double Vmax = Item[hClient - 1].max_measure;
								double A = (double)queued_item->iec_obj.o.type48.sv;
								int error = 0;

								val_to_write = rescale_value_inv(A, Vmin, Vmax, &error);
								if(error){ return;}
							
								sprintf(command_string, "%lf", val_to_write);
							}
							break;
							case C_SE_NB_1:
							{
								double Vmin = Item[hClient - 1].min_measure;
								double Vmax = Item[hClient - 1].max_measure;
								double A = (double)queued_item->iec_obj.o.type49.sv;
								int error = 0;

								val_to_write = rescale_value_inv(A, Vmin, Vmax, &error);
								if(error){ return;}
								
								sprintf(command_string, "%lf", val_to_write);
							}
							break;
							case C_SE_NC_1:
							{
								val_to_write = queued_item->iec_obj.o.type50.sv;
								sprintf(command_string, "%lf", val_to_write);
							}
							break;
							case C_BO_NA_1:
							{
								memset(command_string, 0x00, COMMAND_STR_LEN);
								memcpy(command_string, &(queued_item->iec_obj.o.type51.stcd), sizeof(struct iec_stcd));
							}
							break;
							default:
							{
								//error
								//fprintf(stderr,"Error %d, %s\n",__LINE__, __FILE__);
								//fflush(stderr);

								char show_msg[200];
								sprintf(show_msg, "Error %d, %s\n",__LINE__, __FILE__);
								rfc1006_imp::LogMessage(0, show_msg);


								
								return;
							}
							break;
						}
						
						USES_CONVERSION;

						V_VT(&vCommandValue) = VT_BSTR;

						V_BSTR(&vCommandValue) = SysAllocString(T2COLE(command_string));
						
						if(FAILED(::VariantCopy(&Val[id_of_ItemToWrite], &vCommandValue)))
						{
							//fprintf(stderr,"Error %d, %s\n",__LINE__, __FILE__);
							//fflush(stderr);

							char show_msg[200];
							sprintf(show_msg, "Error %d, %s\n",__LINE__, __FILE__);
							rfc1006_imp::LogMessage(0, show_msg);
							
							return;
						}

						//fprintf(stderr,"Command for sample point %s, value: %s\n", Item[hClient - 1].spname, OLE2T(V_BSTR(&vCommandValue)));
						//fflush(stderr);

						char show_msg[450];
						sprintf(show_msg, "Command for sample point %s, value: %s\n", Item[hClient - 1].spname, OLE2T(V_BSTR(&vCommandValue)));
						LogMessage(NULL, show_msg);
						
						IT_COMMENT2("Command for sample point %s, value: %s\n", Item[hClient - 1].spname, OLE2T(V_BSTR(&vCommandValue)));

						SysFreeString(V_BSTR(&vCommandValue));
					}
					break;
					default:
					{
						V_VT(&vCommandValue) = VT_R4;

						unsigned int v = 0;
						double cmd_val = 0.0;

						switch(queued_item->iec_type)
						{
							case C_SC_TA_1:
							{
								v = queued_item->iec_obj.o.type58.scs;
								cmd_val = (double)v;
							}
							break;
							case C_DC_TA_1:
							{
								v = queued_item->iec_obj.o.type59.dcs;
								cmd_val = (double)v;
							}
							break;
							case C_SE_TA_1:
							{
								double Vmin = Item[hClient - 1].min_measure;
								double Vmax = Item[hClient - 1].max_measure;
								double A = (double)queued_item->iec_obj.o.type61.sv;
								int error = 0;

								cmd_val = rescale_value_inv(A, Vmin, Vmax, &error);
								if(error){ return;}
							}
							break;
							case C_SE_TB_1:
							{
								double Vmin = Item[hClient - 1].min_measure;
								double Vmax = Item[hClient - 1].max_measure;
								double A = (double)queued_item->iec_obj.o.type62.sv;
								int error = 0;

								cmd_val = rescale_value_inv(A, Vmin, Vmax, &error);
								if(error){ return;}
							}
							break;
							case C_SE_TC_1:
							{
								cmd_val = queued_item->iec_obj.o.type63.sv;
							}
							break;
							case C_BO_TA_1:
							{
								memcpy(&v, &(queued_item->iec_obj.o.type64.stcd), sizeof(struct iec_stcd));
								cmd_val = (double)v;
							}
							break;
							case C_SC_NA_1:
							{
								v = queued_item->iec_obj.o.type45.scs;
								cmd_val = (double)v;
							}
							break;
							case C_DC_NA_1:
							{
								v = queued_item->iec_obj.o.type46.dcs;
								cmd_val = (double)v;
							}
							break;
							case C_SE_NA_1:
							{
								double Vmin = Item[hClient - 1].min_measure;
								double Vmax = Item[hClient - 1].max_measure;
								double A = (double)queued_item->iec_obj.o.type48.sv;
								int error = 0;

								cmd_val = rescale_value_inv(A, Vmin, Vmax, &error);
								if(error){ return;}
							}
							break;
							case C_SE_NB_1:
							{
								double Vmin = Item[hClient - 1].min_measure;
								double Vmax = Item[hClient - 1].max_measure;
								double A = (double)queued_item->iec_obj.o.type49.sv;
								int error = 0;

								cmd_val = rescale_value_inv(A, Vmin, Vmax, &error);
								if(error){ return;}
							}
							break;
							case C_SE_NC_1:
							{
								cmd_val = queued_item->iec_obj.o.type50.sv;
							}
							break;
							case C_BO_NA_1:
							{
								memcpy(&v, &(queued_item->iec_obj.o.type51.stcd), sizeof(struct iec_stcd));
								cmd_val = (double)v;
							}
							break;
							default:
							{
								//error
								//fprintf(stderr,"Error %d, %s\n",__LINE__, __FILE__);
								//fflush(stderr);

								char show_msg[200];
								sprintf(show_msg, "Error %d, %s\n",__LINE__, __FILE__);
								rfc1006_imp::LogMessage(0, show_msg);
								
								return;
							}
							break;
						}
						
						V_R4(&vCommandValue) = (float)cmd_val;

						if (FAILED(::VariantCopy(&Val[id_of_ItemToWrite], &vCommandValue)))
						{
							//fprintf(stderr,"Error %d, %s\n",__LINE__, __FILE__);
							//fflush(stderr);

							char show_msg[200];
							sprintf(show_msg, "Error %d, %s\n",__LINE__, __FILE__);
							rfc1006_imp::LogMessage(0, show_msg);
						
							return;
						}

						//fprintf(stderr,"Command for sample point %s, value: %lf\n", Item[hClient - 1].spname, cmd_val);
						//fflush(stderr);

						char show_msg[450];
						sprintf(show_msg, " Command for sample point %s, value: %lf\n", Item[hClient - 1].spname, cmd_val);
						LogMessage(NULL, show_msg);

						//IT_COMMENT2("Command for sample point %s, value: %lf", Item[hClient - 1].spname, cmd_val);
					}
					break;
				}

				if(FAILED(::VariantChangeType(&Val[id_of_ItemToWrite], &Val[id_of_ItemToWrite], 0, V_VT(&Item[hClient - 1]))))
				{
					//fprintf(stderr,"Error %d, %s\n",__LINE__, __FILE__);
					//fflush(stderr);

					char show_msg[200];
					sprintf(show_msg, "Error %d, %s\n",__LINE__, __FILE__);
					rfc1006_imp::LogMessage(0, show_msg);

					return;
				}
								
				DWORD dwAccessRights = Item[hClient - 1].dwAccessRights;

				dwAccessRights = dwAccessRights & OPC_WRITEABLE;

				if(dwAccessRights == OPC_WRITEABLE)
				{
					//rfc1006_imp::g_bWriteComplete = false;

					hr = g_pIOPCAsyncIO2->Write(nWriteItems, hServer, Val, ++g_dwWriteTransID, &g_dwCancelID, &pErrorsWrite);

					if(FAILED(hr))
					{
						LogMessage(hr,"AsyncIO2->Write()");

						return;
					}
					else if(hr == S_FALSE)
					{
						for(dw = 0; dw < nWriteItems; dw++)
						{
							if(FAILED(pErrorsWrite[dw]))
							{
								LogMessage(pErrorsWrite[dw],"AsyncIO2->Write() item returned");

								return;
							}
						}

						::CoTaskMemFree(pErrorsWrite);
					}
					else // S_OK
					{
						::CoTaskMemFree(pErrorsWrite);
					}

					if(V_VT(&Val[id_of_ItemToWrite]) == VT_BSTR)
					{
						SysFreeString(V_BSTR(&Val[id_of_ItemToWrite]));
					}
				}
				else
				{
					IT_COMMENT1("No access write for sample point %s", Item[hClient - 1].spname);
					//fprintf(stderr,"No access write for sample point %s\n", Item[hClient - 1].spname);
					//fflush(stderr);
					
					char show_msg[200];
					sprintf(show_msg, "No access write for sample point %s\n", Item[hClient - 1].spname);
					rfc1006_imp::LogMessage(0, show_msg);
					
					return;
				}
			}
			else
			{
				IT_COMMENT3("Rejeced command for sample point %s, aged for %ld s; max aging time %d s\n", Item[hClient - 1].spname, delta, MAX_COMMAND_SEND_TIME);
				//fprintf(stderr,"Rejeced command for sample point %s, aged for %ld s; max aging time %d s\n", Item[hClient - 1].spname, delta, MAX_COMMAND_SEND_TIME);
				//fflush(stderr);

				char show_msg[200];
				sprintf(show_msg, "Rejeced command for sample point %s, aged for %ld s; max aging time %d s\n", Item[hClient - 1].spname, delta, MAX_COMMAND_SEND_TIME);
				rfc1006_imp::LogMessage(0, show_msg);
			
				return;
			}
		}
		else if(queued_item->iec_type == C_EX_IT_1)
		{
			//Receiving EXIT process command from monitor.exe
			//exit the thread, and stop the process
			fExit = true;
		}
		else if(queued_item->iec_type == C_IC_NA_1)
		{
			//Receiving general interrogation command from monitor.exe
			IT_COMMENT("Receiving general interrogation command from monitor.exe");
			fprintf(stderr,"Receiving general interrogation command from monitor.exe\n");
			fflush(stderr);

			for(dw = 0; dw < g_dwNumItems; dw++)
			{
				hServerRead[dw] = Item[dw].hServer;
			}
			
			// read all items in group

			hr = g_pIOPCAsyncIO2->Read(g_dwNumItems, hServerRead, ++g_dwReadTransID, &g_dwCancelID, &pErrorsRead);

			if(FAILED(hr))
			{
				LogMessage(hr,"AsyncIO2->Read()");
				//When this happen the read is no more working,
				//this means that the General Interrogation is no more working
				//The asyncronous events could still arriving form the server
				//So we exit the process
				fExit = 1;
			}
			else if(hr == S_FALSE)
			{
				//If we arrive here there is something wrong in AddItems()
				for(dw = 0; dw < g_dwNumItems; dw++)
				{
					if(FAILED(pErrorsRead[dw]))
					{
						LogMessage(pErrorsRead[dw],"AsyncIO2->Read() item returned");
					}
				}

				::CoTaskMemFree(pErrorsRead);

				//So we exit the process
				fExit = 1;
			}
			else // S_OK
			{
				::CoTaskMemFree(pErrorsRead);
			}
			/////////end General interrogation command
		}
	}
*/
	return;
}

void rfc1006_imp::alloc_command_resources(void)
{
	/*
	hServerRead = (OPCHANDLE*)calloc(1, g_dwNumItems*sizeof(OPCHANDLE));
		
	DWORD dw = 0;
	DWORD nWriteItems = ITEM_WRITTEN_AT_A_TIME;

	::VariantInit(&vCommandValue);

	for(dw = 0; dw < nWriteItems; dw++)
	{
		id_of_ItemToWrite = dw;
		::VariantInit(&Val[dw]);
	}
	*/
}

void rfc1006_imp::free_command_resources(void)
{
	/*
	DWORD dw = 0;
	DWORD nWriteItems = ITEM_WRITTEN_AT_A_TIME;

	for(dw = 0; dw < nWriteItems; dw++)
	{
		::VariantClear(&Val[dw]);
	}
	
	::VariantClear(&vCommandValue);
	*/
}
