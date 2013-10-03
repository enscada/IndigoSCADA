/*
 *                         IndigoSCADA
 *
 *   This software and documentation are Copyright 2002 to 2013 Enscada 
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
#include "modbus_item.h"
#include "modbus_imp.h"
#include "stdlib.h"

#define MAX_KEYLEN 256
#define MAX_COMMAND_SEND_TIME 60

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
	modbus_imp * cl = (modbus_imp*)recvCallBackParam;
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
iec_item_type modbus_imp::instanceSend;
ORTEPublication* modbus_imp::publisher = NULL;
////////////////////////////////Middleware/////////////////

//   
//  Class constructor.   
//   
modbus_imp::modbus_imp(struct modbusContext* my_ctx, char* line_number, int polling_time):
fExit(false),pollingTime(polling_time), general_interrogation(true), is_connected(false)
{   
	lineNumber = atoi(line_number);
	my_modbus_context.use_context = my_ctx->use_context;
	my_modbus_context.server_id = my_ctx->server_id;

	if(my_modbus_context.use_context == TCP)
	{
		strcpy(my_modbus_context.modbus_server_address, my_ctx->modbus_server_address);
		strcpy(my_modbus_context.modbus_server_port, my_ctx->modbus_server_port);
	}
	else if(my_modbus_context.use_context == RTU)
	{
		strcpy(my_modbus_context.serial_device, my_ctx->serial_device);
		my_modbus_context.baud = my_ctx->baud;
		my_modbus_context.data_bit = my_ctx->data_bit;
		my_modbus_context.stop_bit = my_ctx->stop_bit;
		my_modbus_context.parity = my_ctx->parity;
	}

	if(my_modbus_context.use_context == TCP) 
	{
        ctx = modbus_new_tcp(my_modbus_context.modbus_server_address, atoi(my_modbus_context.modbus_server_port));
    } 
	else 
	{
        ctx = modbus_new_rtu(my_modbus_context.serial_device, my_modbus_context.baud, my_modbus_context.parity, my_modbus_context.data_bit, my_modbus_context.stop_bit);
    }

    if (ctx != NULL) 
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
		strcat(fifo_monitor_name, "modbus");

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
		strcat(fifo_control_name, "modbus");

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
	}
	else
	{
        fprintf(stderr, "Unable to allocate libmodbus context\n");
		fExit = 1;
		return;
    }

	modbus_set_debug(ctx, TRUE);

    modbus_set_error_recovery(ctx,(modbus_error_recovery_mode)(MODBUS_ERROR_RECOVERY_LINK | MODBUS_ERROR_RECOVERY_PROTOCOL));

    modbus_set_slave(ctx, my_modbus_context.server_id);

    if (modbus_connect(ctx) == -1) 
	{
        fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
        //modbus_free(ctx);
		//fExit = 1;
        //return;
		is_connected = false;
    }
	else
	{
		is_connected = true;
	}
}   
//   
//  Class destructor.   
//   
modbus_imp::~modbus_imp()  
{   
    // free resources   
	fExit = 1;
    return;   
}   

static u_int n_msg_sent = 0;

int modbus_imp::PollServer(void)
{
	IT_IT("modbus_imp::PollServer");
	
	int rc = 0;

	/* Allocate and initialize the memory to store the bits */
	#define MAX_BITS_IN_MEMORY_BLOCK 30

	nb_points = MAX_BITS_IN_MEMORY_BLOCK;
	tab_rp_bits = (uint8_t *) malloc(nb_points * sizeof(uint8_t));

	memset(tab_rp_bits, 0x00, nb_points * sizeof(uint8_t));

	#define MAX_REGISTERS_IN_MEMORY_BLOCK 30

	/* Allocate and initialize the memory to store the registers */
	nb_points = MAX_REGISTERS_IN_MEMORY_BLOCK;

	tab_rp_registers = (uint16_t *) malloc(nb_points * sizeof(uint16_t));

	memset(tab_rp_registers, 0x00, nb_points * sizeof(uint16_t));

	////////////General interrogation condition//////////////
	general_interrogation = true;
	loops = 0;
	//////////////////////////////////////////////////////////
  
	while(true) //the polling loop
	{	
		if(is_connected)
		{
			rc = PollItems();

			loops++;

			if(loops == 4)
			{
				general_interrogation = false;
			}
		
			if(rc)
			{ 
				fprintf(stderr,"modbus on line %d exiting...., due to lack of connection with server\n", lineNumber);
				fflush(stderr);

				IT_COMMENT("modbus_imp exiting...., due to lack of connection with server");
				
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

				n_msg_sent++;
			
				//break; //this terminate the loop and the program

				is_connected = false;

				modbus_close(ctx);
			}
		}
		else
		{
			//Try to reconnect
			if (modbus_connect(ctx) == -1) 
			{
				fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
			}
			else
			{
				is_connected = true;

				////////////General interrogation condition//////////////
				general_interrogation = true;
				loops = 0;
				//////////////////////////////////////////////////////////
			}
		}

		if(fExit)
		{
			IT_COMMENT("Terminate modbus loop!");
			break;
		}

		#define USE_KEEP_ALIVE_WATCH_DOG

		#ifdef USE_KEEP_ALIVE_WATCH_DOG
		gl_timeout_connection_with_parent++;

		if(gl_timeout_connection_with_parent > 1000*60/pollingTime)
		{
			break; //exit loop for timeout of connection with parent
		}
		#endif
				
		::Sleep(pollingTime);
	}
	
	IT_EXIT;
	return 0;
}

int modbus_imp::Start(void)
{
	IT_IT("modbus_imp::Start");
	
	char show_msg[200];
	sprintf(show_msg, " IndigoSCADA MODBUS master Start\n");
	LogMessage(NULL, show_msg);

	if(fExit == 1)
	{
		return(1); //error
	}
	
	if(AddItems())
	{
		return(1); //error
	}

	IT_EXIT;
    return(0);
}

int modbus_imp::Stop()
{
	IT_IT("modbus_imp::Stop");

	fprintf(stderr,"Entering Stop()\n");
	fflush(stderr);

	/* Free the memory */
    free(tab_rp_bits);
    free(tab_rp_registers);

    /* Close the connection */
    modbus_close(ctx);
    modbus_free(ctx);

	while(received_command_callback)
	{
		Sleep(100);
	}
	
	// terminate server and it will clean up itself

	char show_msg[200];
	sprintf(show_msg, " IndigoSCADA MODBUS master End\n");
	LogMessage(NULL, show_msg);

	IT_EXIT;
	return 1;
}

struct log_message{

	int ioa;
	char message[150];
};

void modbus_imp::LogMessage(int* error, const char* name)
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

void modbus_imp::get_utc_host_time(struct cp56time2a* time)
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

time_t modbus_imp::epoch_from_cp56time2a(const struct cp56time2a* time)
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

#define ABS(x) ((x) >= 0 ? (x) : -(x))

//Retun 1 on error
int modbus_imp::PollItems(void)
{
	IT_IT("modbus_imp::PollItems");

	struct iec_item item_to_send;
	struct cp56time2a actual_time;
	////////////////////////////////Start protocol implementation///////////////////////////////////
	int rc;
    bool send_item;
	int bit_size;
    	
    comm_error_counter = 0;

	for(int rowNumber = 0; rowNumber < db_n_rows; rowNumber++)
	{
		memset(&item_to_send,0x00, sizeof(struct iec_item));

		memset(tab_rp_bits, 0x00, nb_points * sizeof(uint8_t));

		memset(tab_rp_registers, 0x00, nb_points * sizeof(uint16_t));

		/* Function codes */
		if(Config_db[rowNumber].modbus_function_read == FC_READ_COILS)
		{
			//0x01
				
			if(Config_db[rowNumber].modbus_type == VT_BOOL)
			{
				bit_size = 1;

				int address = Config_db[rowNumber].modbus_start_address;

				rc = modbus_read_bits(ctx, address, bit_size, tab_rp_bits);

				if (rc != 1) 
				{
                    comm_error_counter++;
					
                    continue;
				}

				uint8_t value = tab_rp_bits[0];

				printf("modbus_read_bits: value = %d\n", (int)value);

				if(Config_db[rowNumber].last_value.a != value)
				{
					Config_db[rowNumber].last_value.a = value;

					send_item = true;
				}
				else
				{
					send_item = false;
				}

				item_to_send.iec_obj.ioa = Config_db[rowNumber].ioa_control_center;

				item_to_send.cause = 0x03;
			
				item_to_send.iec_type = M_SP_TB_1;
				
				get_utc_host_time(&actual_time);

				item_to_send.iec_obj.o.type30.sp = value;
				item_to_send.iec_obj.o.type30.time = actual_time;
				item_to_send.iec_obj.o.type30.iv = 0;
				
				IT_COMMENT1("Value = %d", value);
			}
			else
			{
				printf("Modbus type %d not supported with FC_READ_COILS", Config_db[rowNumber].modbus_type);
			}
		}
		else if(Config_db[rowNumber].modbus_function_read == FC_READ_DISCRETE_INPUTS)
		{
			//0x02

			if(Config_db[rowNumber].modbus_type == VT_BOOL)
			{
				bit_size = 1;

				int address = Config_db[rowNumber].modbus_start_address;

				rc = modbus_read_input_bits(ctx, address, bit_size, tab_rp_bits);

				if (rc != 1) 
				{
                    comm_error_counter++;
					
                    continue;
				}

				uint8_t value = tab_rp_bits[0];

				printf("modbus_read_input_bits: value = %d\n", (int)value);

				if(Config_db[rowNumber].last_value.a != value)
				{
					Config_db[rowNumber].last_value.a = value;

					send_item = true;
				}
				else
				{
					send_item = false;
				}

				item_to_send.iec_obj.ioa = Config_db[rowNumber].ioa_control_center;

				item_to_send.cause = 0x03;
			
				item_to_send.iec_type = M_SP_TB_1;
				
				get_utc_host_time(&actual_time);

				item_to_send.iec_obj.o.type30.sp = value;
				item_to_send.iec_obj.o.type30.time = actual_time;
				item_to_send.iec_obj.o.type30.iv = 0;
				
				IT_COMMENT1("Value = %d", value);
			}
			else
			{
				printf("Modbus type %d not supported with FC_READ_COILS", Config_db[rowNumber].modbus_type);
			}
		}
		else if(Config_db[rowNumber].modbus_function_read == FC_READ_HOLDING_REGISTERS)
		{
			//0x03
			if((Config_db[rowNumber].modbus_type == VT_I4) || (Config_db[rowNumber].modbus_type == VT_R4))
			{
				int registers = 2; //read 32 bits

				int address = Config_db[rowNumber].modbus_start_address;

				rc = modbus_read_registers(ctx, address, registers, tab_rp_registers);

				printf("modbus_read_registers: ");

				if (rc != registers) 
				{
                    comm_error_counter++;
					
                    continue;
				}

				if(Config_db[rowNumber].iec_type_read == M_ME_TF_1)
				{
					float real;

					real = modbus_get_float(tab_rp_registers);

					printf("Get float: %f\n", real);

					if(ABS(Config_db[rowNumber].last_value.f - real) > Config_db[rowNumber].deadband)
					{
						Config_db[rowNumber].last_value.f = real;

						send_item = true;
					}
					else
					{
						send_item = false;
					}

					item_to_send.iec_obj.ioa = Config_db[rowNumber].ioa_control_center;

					item_to_send.cause = 0x03;

					item_to_send.iec_type = M_ME_TF_1;
					
					get_utc_host_time(&actual_time);

					item_to_send.iec_obj.o.type36.mv = real;
					item_to_send.iec_obj.o.type36.time = actual_time;
					item_to_send.iec_obj.o.type36.iv = 0;
				}
				else if(Config_db[rowNumber].iec_type_read == M_IT_TB_1)
				{
					int integer32;
					integer32 = modbus_get_int(tab_rp_registers);

					printf("Get integer: %d\n", integer32);

					if(ABS(Config_db[rowNumber].last_value.a - integer32) > (int)Config_db[rowNumber].deadband)
					{
						Config_db[rowNumber].last_value.a = integer32;

						send_item = true;
					}
					else
					{
						send_item = false;
					}

					item_to_send.iec_obj.ioa = Config_db[rowNumber].ioa_control_center;

					item_to_send.cause = 0x03;

					item_to_send.iec_type = M_IT_TB_1;
					
					get_utc_host_time(&actual_time);

					item_to_send.iec_obj.o.type37.counter = integer32;
					item_to_send.iec_obj.o.type37.time = actual_time;
					item_to_send.iec_obj.o.type37.iv = 0;
				}
			}
			else if(Config_db[rowNumber].modbus_type == VT_I2)
			{
				int registers = 1; //read 16 bits

				int address = Config_db[rowNumber].modbus_start_address;

				rc = modbus_read_registers(ctx, address, registers, tab_rp_registers);
				printf("modbus_read_registers: ");

				if (rc != registers) 
				{
                    comm_error_counter++;
					
                    continue;
				}

				if(Config_db[rowNumber].iec_type_read == M_ME_TE_1)
				{
					short integer16;
					integer16 = tab_rp_registers[0];

					printf("Get integer: %d\n", integer16);

					if(ABS(Config_db[rowNumber].last_value.a - integer16) > (short)Config_db[rowNumber].deadband)
					{
						Config_db[rowNumber].last_value.a = integer16;

						send_item = true;
					}
					else
					{
						send_item = false;
					}

					item_to_send.iec_obj.ioa = Config_db[rowNumber].ioa_control_center;

					item_to_send.cause = 0x03;

					item_to_send.iec_type = M_ME_TE_1;
					
					get_utc_host_time(&actual_time);

					item_to_send.iec_obj.o.type35.mv = integer16;
					item_to_send.iec_obj.o.type35.time = actual_time;
					item_to_send.iec_obj.o.type35.iv = 0;

				}
				else if(item_to_send.iec_type = M_SP_TB_1)
				{
					short integer16;
					integer16 = tab_rp_registers[0];

					//uint8_t value = get_bit_from_word(integer16, Config_db[rowNumber].offset);				
					//get a bit value from a word
					uint8_t value = integer16&(1 << Config_db[rowNumber].offset)  ? 1 : 0;				
					
					printf("get bit from word: value = %d\n", (int)value);

					if(Config_db[rowNumber].last_value.a != value)
					{
						Config_db[rowNumber].last_value.a = value;

						send_item = true;
					}
					else
					{
						send_item = false;
					}
					
					item_to_send.iec_obj.ioa = Config_db[rowNumber].ioa_control_center;

					item_to_send.cause = 0x03;
				
					item_to_send.iec_type = M_SP_TB_1;
					
					get_utc_host_time(&actual_time);

					item_to_send.iec_obj.o.type30.sp = value;
					item_to_send.iec_obj.o.type30.time = actual_time;
					item_to_send.iec_obj.o.type30.iv = 0;
					
					IT_COMMENT1("Value = %d", value);
				}
			}
			else
			{
				printf("Modbus type %d not supported with FC_READ_HOLDING_REGISTERS", Config_db[rowNumber].modbus_type);
			}
		}
		else if(Config_db[rowNumber].modbus_function_read == FC_READ_INPUT_REGISTERS)
		{
			//0x04
			printf("Function %x not supported\n", 0x04);
		
		}
		else if(Config_db[rowNumber].modbus_function_read == FC_READ_EXCEPTION_STATUS)
		{
			//0x07
			printf("Function %x not supported\n", 0x07);
		
		}
		else if(Config_db[rowNumber].modbus_function_read == FC_REPORT_SLAVE_ID)
		{
			//0x11
			printf("Function %x not supported\n", 0x11);
		}
		else if(Config_db[rowNumber].modbus_function_read == FC_WRITE_AND_READ_REGISTERS)
		{
			//0x17
			printf("Function %x not supported\n", 0x17);
		}
		else
		{
			printf("Function not supported\n");
		}
		
		if(send_item || general_interrogation)
		{
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
		}
	}

    if(comm_error_counter >= db_n_rows)
    {
        IT_EXIT; //Lost connection with server...
	    return 1;
    }

	IT_EXIT;
	return 0;

}

#define _EPSILON_ ((double)(2.220446E-16))

#define DO_NOT_RESCALE

short modbus_imp::rescale_value(double V, double Vmin, double Vmax, int* error)
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

double modbus_imp::rescale_value_inv(double A, double Vmin, double Vmax, int* error)
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


void modbus_imp::check_for_commands(struct iec_item *queued_item)
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

			/////////Here we execute the QUERY:////////////////////////////////////////// /////////////////////////////
			// select from Config_db table the rowNumber where ioa is equal to ioa of packet arriving (command) from monitor.exe
			///////////////////////////////////////////////////////////////////////////////////////
			int found = 0;
			DWORD rowNumber = -1;

			for(int dw = 0; dw < db_n_rows; dw++) 
			{ 
				if(queued_item->iec_obj.ioa == Config_db[dw].ioa_control_center)
				{
					found = 1;
					rowNumber = dw;
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
			
			//check iec type of command
			//if(Config_db[rowNumber].iec_type_write != queued_item->iec_type)
			//{
			//	//error
			//	fprintf(stderr,"Error: Command with IOA %d has iec_type %d, different from IO list type %d\n", queued_item->iec_obj.ioa, queued_item->iec_type, Config_db[rowNumber].iec_type_write);
			//	fflush(stderr);
			//	fprintf(stderr,"Command NOT executed\n");
			//	fflush(stderr);
			//	return;
			//}
			
			//Receive a write command
								
			fprintf(stderr,"Receiving command for ioa %d\n", queued_item->iec_obj.ioa);
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
					modbus_imp::LogMessage(0, show_msg);
				
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

			if(delta < MAX_COMMAND_SEND_TIME && de   ;
t�׋D$TƄ$t  �J����u��B�L$T;t�׋D$Ƅ$t  �J����u��B�L$;
t�׋D$Ǆ$t  �����J������  �L$��B;��  ��B�  ��B���u��B�D$�A��Ex�   ;�Ǆ$t  )   |U�X   �M|h ���C�u|3���T$$R�F8�F<�F@��C����NH�N�P�FP ��VL�P�ExC��X;�~���I�Ӌ�����   �=�B�5�B���D$ �D$    ��  h��L$8��Ƅ$t  *�Ӌ�L$4j Q�����   ���D$4Ƅ$t  )�J����u��B�L$4;
t��B����   ;ux��   h���$�   ��Ƅ$t  +�Ӌ��$�   Q��$�   Q�����   �M|��PƄ$x  ,�V����A��$�   Ƅ$t  +�J����u��B��$�   ;
t��B��$�   Ƅ$t  )�J����u��B��$�   ;t��B�D$��th��L$��Ch��L$h��Ƅ$t  -�Ӌ�L$dQ��$�   Q�����   P��$�   h�RƄ$�  .�$Ch�P��$�   Ƅ$�  /P�,C��P�L$Ƅ$x  0�@C��$�   Ƅ$t  /�J����u��B��$�   ;
t
�5�B����5�B��$�   Ƅ$t  .�J����u��B��$�   ;t�֋�$�   Ƅ$t  -�J����u��B��$�   ;
t�֋D$dƄ$t  )�J����u��B�L$d;t�֋t$F�t$�Ӌ�����   ;t$ ������5�B�D$��$�   PhtQ�$Ch��T$$PRƄ$�  1�,C����$�   Ƅ$t  3�J����u��B��$�   ;t��h �L$D��h �L$<Ƅ$x  4��h �L$pƄ$x  5��h ��$�   Ƅ$x  6��Ƅ$t  7�ӍL$@�Q�L$<Q�L$tQ��$�   Q�L$$jQU���Rx��$�   Ƅ$t  6�J����u��B��$�   ;
t�֋D$lƄ$t  5�J����u��B�L$l;t�֋D$8Ƅ$t  4�J����u��B�L$8;
t�֋D$@Ƅ$t  3�J����u��B�L$@;t�֋D$Ƅ$t  )�J����u��B�L$;
t�֋D$Ǆ$t  �����J������  ��B�L$;��  ���  �=�I�׋�����   ���x  �D$�Mx�   ;ȉD$��   �X   hl�L$T��BǄ$t  8   �׋�L$PQ��$�   Q�����   �U|P�Ƅ$x  9R��C�؋�$�   ��Ƅ$t  8�J����u��B��$�   ;t��B�D$PǄ$t  �����J����u��B�L$P;
t��B��u�D$�Mx@��X;��D$�:����׋�����   �D$H�D$�����u
  �=�I�׋�����   ���[
  �5�Bhd�L$\��Ǆ$t  :   �׋�L$XQ��$�   Q�����   �T$PRƄ$|  ;�(I����$�   �=�BƄ$t  >�J����u��B��$�   ;t�׋D$XƄ$t  =�J����u��B�L$X;
t�׍D$jP��$D  ��C�|C��$<  �Ex��$<  PƄ$x  ?�xC���   ��$<  P�tC�=pC���   R��$@  �׍��   ��$<  P�׍��   Q��$@  �׍��   ��$<  S�׍��   ��$<  R�׍��   ��$<  P�׍��   Q��$@  �׍��   ��$<  R�׋��C�����3����Iu���   �
ǅ�      �E|3�;��Eh   t �H��x�h�9 QjXP苗  W��  ���]|�}xG���W��   Q蠘  ���D$$;�Ƅ$t  @th�9 hp� �XWjXS�8聘  �E5Ƅ$t  ?���]|t
ǅ�   �  ���d  ���L  j ��$�   h<R�8������,I�D$0WPƄ$�  A��(�Ӄ���$�   h�	  Q��Ƅ$|  B�lC�D$��$�   URƄ$|  C�Ӄ���$  h�	  Q��Ƅ$|  D�lC�؍�$  Ƅ$t  ER�0IP��$  h PƄ$�  F�$Ch��$  PQƄ$�  G�,CS��$,  PRƄ$�  H�(ChdP��$4  Ƅ$�  IP�,C�L$P��$,  QPRƄ$�  J�(C��@hP�D$Ƅ$|  KP�,C����$�   Ƅ$t  V��B��$  Ƅ$t  U��B��$  Ƅ$t  T��B��$   Ƅ$t  S��B��$  Ƅ$t  R��B��$  Ƅ$t  Q��B��$  Ƅ$t  P��B��$�   Ƅ$t  O��B��$�   Ƅ$t  N��B�M�L$$��$t  ��Bh �L$ ��h �L$4Ƅ$x  W��h �L$0Ƅ$x  X��h �L$Ƅ$x  Y��Ƅ$t  Z�4I�L$�Q�L$4Q�L$4Q�L$Qj �L$(Qj ���Rx�L$Ƅ$t  Y��B�L$,Ƅ$t  X��B�L$0Ƅ$t  W��B�L$��$t  ��B�TI����   h �L$��h �L$0Ƅ$x  [��h �L$4Ƅ$x  \��h �L$ Ƅ$x  ]��Ƅ$t  ^�TI�L$�Q�L$0Q�L$8Q�L$(Q�L$$j Qj ���Rx�L$Ƅ$t  ]��B�L$0Ƅ$t  \��B�L$,Ƅ$t  [��B�L$��$t  ��Bh�T$ UR�,CWP�D$4Ƅ$�  _P�(C���L$Ƅ$t  a��B�XI��j j �L$(�>�CPjx���WX�L$ ��$t  ��B�L$Ƅ$t  A��B��$�   Ƅ$t  ?�J����u��B��$�   ;
t��B��$<  Ƅ$t  =�hC�D$Ǆ$t  �����J������  �L$������5�I�֋�����   �D$ �֋�����   ���k  h��L$d��BǄ$t  b   �֋�L$`Q��$�   Q�����   �T$PRƄ$|  c�(I����$�   �5�BƄ$t  f�J����u��B��$�   ;t�֋D$`Ƅ$t  e�J����u��B�L$`;
t�֋D$ �5C�D$$    �D$(    �X���֋����3����Itp����P�t@�\$(���L$hhd��B�L$(�T$$Q�D$lR�L$PQ��Ƅ$�  g�\I�D$hƄ$t  e�J����u��B�L$h;
t��B�L$�֋L$$�T$ ���D$(PQ�J��PWh��h@�D$$��Ǆ$t  �����J������  �L$�����5�I�֋�����   �D$�֋�����   ����  h��L$t��BǄ$t  h   �֋�L$pj Q�����   ���D$pǄ$t  �����J����u��B�L$p;
t��B�D$�CǄ$�       Ǆ$�       �H�Ӌ����3����It�L$����P�t@ݜ$�   ����$�   ��$�   �L$RPV����Ph��h@݄$�   �   3���$,  ���T$$��f�ٜ$  RƄ$  ?��$  �.  �D$$f�L$(��$"  ���   �T$*��$.  @j"���   ��$  Pf��$.  ��$0  �8������   �ع	   3�����$!  󫊌$   ���   �
��$6  ���   �   ��$A  P󥉕�   ���   蔎  ����$l  _^][d�    ��h  � �� �� �� x �� Z� ������������j�h�/d�    Pd�%    Q��BV�� �t$��u��B��A���I�N�D$    ��I�L$�FH    �FL    ��^d�    ��Ð�VW�|$��G��t!Hu.�G�N(PQ�  ����W��H_^� �W�F(RP�   ��W����H_^� �j�h�0d�    Pd�%    ��<SU�l$XV�5,IW�D$4UP�փ��=lC�L$0h�	  Q���D$\    �׋؋T$\�D$,RP�D$\�փ��L$(h�	  Q���D$\�׋��T$$�D$TR�0IP�D$(h P�D$d�$Ch�L$0PQ�D$p�,CV�T$8PR�D$|�(ChdP�D$DƄ$�   P�,CS�L$HPQƄ$�   �(C��@h�D$X	P�T$hR�,C���D$�5�B�D$T�J����u��B�L$;t�֋D$�D$T�J����u��B�L$;
t�֋D$�D$T�J����u��B�L$;t�֋D$�D$T�J����u��B�L$;
t�֋D$ �D$T�J����u��B�L$ ;t�֋D$$�D$T�J����u��B�L$$;
t�֋D$(�D$T�J����u��B�L$(;t�֋D$,�D$T�J����u��B�L$,;
t�֋D$0�D$T�J����u��B�L$0;t�֋D$4��\$T�J����u��B�L$4;
t�֋5�Bh �L$H��h �L$D�D$X��h �L$@�D$X��h �L$<�D$X���D$T�4I�L$D�Q�L$DQ�L$DQ�L$DQ�L$pj Qj ���Rx�L$8�D$T��B�L$<�D$T��B�L$@�D$T��B�L$D�\$T��B�=TI�ׅ���   h �L$<��h �L$@�D$X��h �L$D�D$X��h �L$H�D$X���D$T�׍L$8�Q�L$@Q�L$HQ�L$PQ�L$pj Qj ���Rx�L$D�D$T��B�L$@�D$T��B�L$<�D$T��B�L$8�\$T��B�T$\h�D$`RP�,CU�L$XPQ�D$l�(C���L$\�D$T��B�XI��j j �L$P�>�CPjx���WX�L$H�\$T��B�L$`�D$T������B�L$L_^][d�    ��HÐ�������������j�h�1d�    Pd�%    ��<SU�l$XV�5,IW�D$4UP�փ��=lC�L$0h�	  Q���D$\    �׋؋T$\�D$,RP�D$\�փ��L$(h�	  Q���D$\�׋��T$$�D$TR�0IP�D$(htP�D$d�$Ch�L$0PQ�D$p�,CV�T$8PR�D$|�(ChdP�D$DƄ$�   P�,CS�L$HPQƄ$�   �(C��@h�D$X	P�T$hR�,C���D$�5�B�D$T�J����u��B�L$;t�֋D$�D$T�J����u��B�L$;
t�֋D$�D$T�J����u��B�L$;t�֋D$�D$T�J����u��B�L$;
t�֋D$ �D$T�J����u��B�L$ ;t�֋D$$�D$T�J����u��B�L$$;
t�֋D$(�D$T�J����u��B�L$(;t�֋D$,�D$T�J����u��B�L$,;
t�֋D$0�D$T�J����u��B�L$0;t�֋D$4��\$T�J����u��B�L$4;
t�֋5�Bh �L$H��h �L$D�D$X��h �L$@�D$X��h �L$<�D$X���D$T�4I�L$D�Q�L$DQ�L$DQ�L$DQ�L$pj Qj ���Rx�L$8�D$T��B�L$<�D$T��B�L$@�D$T��B�L$D�\$T��B�=TI�ׅ���   h �L$<��h �L$@�D$X��h �L$D�D$X��h �L$H�D$X���D$T�׍L$8�Q�L$@Q�L$HQ�L$PQ�L$pj Qj ���Rx�L$D�D$T��B�L$@�D$T��B�L$<�D$T��B�L$8�\$T��B�T$\h�D$`RP�,CU�L$XPQ�D$l�(C���L$\�D$T��B�XI��j j �L$P�>�CPj{���WX�L$H�\$T��B�L$`�D$T������B�L$L_^][d�    ��HÐ��������������A8V�q8�����t+�v��t�6�D$�NL^;���� �D$3��NL^;���� 2�^� VW���G8�w8�����v����Cj ����C�GX�H�ΉGX�_^Ð������������T$SUVW�����3��t$��I���ًL$�����̓��3Ʌ�~�< t�FA;�|�_^][Ð���������$S�ًClH����   �$��� �Cl   [��$�VW�   3��|$j"�f����   �D$d�D$&@���   �D$P�D$    �D$��z�����   �й	   3����T$5�L$�D$*����   �   �t$󥋋�   ���   Q���   �2�  ���Cl   _^[��$��Cl   [��$Ë��   [��$ÍI �� � �� �� �� U����j�h�2d�    Pd�%    ��   SUV��Wj��  �D$Pj"PQ�D$4    躂  �����
  �0C�=�H�~l��  �|$L���  ��B���u��B�D$�(E�(���   �L$@Ǆ$�       Ph�Q�Ӄ��T$��j R��Hh��L$<��Bj ��$�   h�PƄ$�   �׋�j ��$�   h�QƄ$�   �׍N(��$�   QPRƄ$�   �(CUP��$�   Ƅ$�   P�(C�L$H�T$xQPRƄ$�   �(CP�D$xPƄ$  ������$�   ��DƄ$�   �J����u��B�L$H;
t��B��$�   Ƅ$�   ��B�L$pƄ$�   ��B�L$xƄ$�   ��B��$�   Ƅ$�   ��B�L$8Ƅ$�    ��B�F4����   j ��$�   h�P�׋�j ��$�   h�QƄ$�   �׍N(��$�   QPRƄ$�   �(CUP��$�   Ƅ$�   	P�(C�L$H��$�   QPRƄ$�   
�(C��<P��Ƅ$�   ��H��$�   Ƅ$�   
��B��$�   Ƅ$�   	��B��$�   Ƅ$�   ��B��$�   Ƅ$�   ��B��$�   Ƅ$�    ��B�D$�Fl   Ǆ$�   �����J����u��B�L$;t��B���   �T$b�-h@AQRh��ՍD$Xj"P��v������tj�@��B���u��B�D$�A��T$LǄ$�      ���   �B�=�   ��  3Ɋ�h� �$�8� �T$Q�D$��RhTP�  �L$Q�T$��QhTR�  �D$QP�l  �D$Q���L$�$hTQ�Ӄ��]  �T$QR�D$hTP�C  �D$Q���L$�$h�Q�Ӄ��*  �T$U�D$QRP�L$h�Q�Ӄ��  �T$Q�D$RhTP��  hl�Ճ���  �~l��  ���   AQh(�Ջ�B�����u��B�D$�(E�(���   �L$@Ƅ$�   Ph�Q�Ӄ��T$��jR��Hh��L$D��Bj ��$�   h�PƄ$�   �׋�j ��$�   h�QƄ$�   �׍N(��$�   QPRƄ$�   �(CUP��$�   Ƅ$�   P�(C�L$L�T$`QPRƄ$�   �(CP��$�   PƄ$  ������D$t��DƄ$�   �J����u��B�L$0;
t��B��$�   Ƅ$�   ��B�L$tƄ$�   ��B�L$|Ƅ$�   ��B��$�   Ƅ$�   ��B�L$@Ƅ$�   ��B�F4����   j ��$�   h�P�׋�j ��$�   h�QƄ$�   �׍N(��$�   QPRƄ$�   �(CUP��$�   Ƅ$�   P�(C�L$L��$�   QPRƄ$�   �(C��<P��Ƅ$�   ��H��$�   Ƅ$�   ��B��$�   Ƅ$�   ��B��$�   Ƅ$�   ��B��$�   Ƅ$�   ��B��$�   Ƅ$�   ��B�D$�Fl   Ƅ$�   �J����u0��B�L$;t#��B�h��Ճ�j �L$hTQ�Ӄ���B���u��B�D$�B��D$M�L$PhTQƄ$�   �ӍT$ �D$PRh�P�$Ch��L$XPQƄ$�   �,C�N(�T$XQPRƄ$�   �(ChlP�D$XƄ$�   P�,C��<�D$4Ƅ$�    �J����u��B�L$4;
t
�-�B����-�B�D$<Ƅ$�   �J����u��B�L$<;t�ՋD$DƄ$�   �J����u��B�L$D;
t�Ջ-�Bh �L$0��h �L$,Ƅ$�   !��Ƅ$�   "��I�L$,�Q�L$,Q�L$Q�L$Q�L$0jQV���Rx�D$(Ƅ$�   !�J����u��B�L$(;
t
�-�B����-�B�D$,Ƅ$�   �J����u��B�L$,;t�ՋD$$Ƅ$�   ��2�D$ ���   J����u��B�L$ ;
t�ՋD$�