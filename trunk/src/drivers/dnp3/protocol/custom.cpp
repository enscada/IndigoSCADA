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

//////////////////////////apa+++ 19-06-2012///////////
#include "stdint.h"
#include "iec104types.h"
#include "iec_item.h"
#include "clear_crc_eight.h"
#include "iec_item_type.h" //Middleware
//////////////////////////////////////////////////////

#include <stdio.h>
#include "assert.h"
#include "custom.hpp"

// Event Interface -------------------

CustomDb::CustomDb() :
  numInits(0),
  n_sent_items(0)
{

}

////////////////////////////////////////////apa+++////////////////////////////////////////////
#define ABS(x) ((x) >= 0 ? (x) : -(x))

/////////////////////////////////Globals remove ASAP/////////////
extern iec_item_type* global_instanceSend;
extern ORTEPublication* global_publisher;
/////////////////////////////////////////////////////////////////

#define QUALITY_ONLINE 0x01

#define OFFSET_AI 0
#define OFFSET_BI 100
#define OFFSET_CI 200
#define OFFSET_AO 300
#define OFFSET_BO 400

void CustomDb::changePoint(   DnpAddr_t      addr,
			     DnpIndex_t     index,
		             PointType_t    pointType,
			     float            value,
			     DnpTime_t      timestamp,
				 uint8_t flag)
{
    assert(addr != 0);
    
	cp56time2a time;
	struct iec_item item_to_send;
	double delta = 0.0;
    	
	memset(&item_to_send,0x00, sizeof(struct iec_item));
	
	item_to_send.iec_obj.ioa = 0;
	//item_to_send.cause = cot;
		
	switch(pointType)
	{
		case EventInterface::AI:
		{
			printf("AI\n");

			item_to_send.iec_obj.ioa = index + OFFSET_AI + 1;

			printf("ioa = %d\n", item_to_send.iec_obj.ioa);

			item_to_send.iec_type = M_ME_TF_1;

			//DNP time (DnpTime_t) is a six byte unsigned int representing the number of milli-seconds
			// since midnight UTC Jan 1, 1970 (does not include leap seconds)
			
			epoch_to_cp56time2a(&time, timestamp);
			item_to_send.iec_obj.o.type36.mv = value;

			printf("value = %d\n", value);
			
			item_to_send.iec_obj.o.type36.time = time;

			if((flag & QUALITY_ONLINE) != QUALITY_ONLINE)
				item_to_send.iec_obj.o.type36.iv = 1;
		}
		break;
		case EventInterface::BI:
		{
			printf("BI\n");

			item_to_send.iec_obj.ioa = index + OFFSET_BI + 1; //Index on RTU starts from 0, while IOA on control center starts from 1

			printf("ioa = %d\n", item_to_send.iec_obj.ioa);

			item_to_send.iec_type = M_SP_TB_1;
			epoch_to_cp56time2a(&time, timestamp);
			item_to_send.iec_obj.o.type30.sp = value;
			item_to_send.iec_obj.o.type30.time = time;

			printf("value = %d\n", value);

			if((flag & QUALITY_ONLINE) != QUALITY_ONLINE)
				item_to_send.iec_obj.o.type30.iv = 1;
		}
		break;
		case EventInterface::CI:
		{
			printf("CI\n");

			item_to_send.iec_obj.ioa = index + OFFSET_CI + 1;

			printf("ioa = %d\n", item_to_send.iec_obj.ioa);

			item_to_send.iec_type = M_IT_TB_1;
			epoch_to_cp56time2a(&time, timestamp);
			item_to_send.iec_obj.o.type37.counter = value;
			item_to_send.iec_obj.o.type37.time = time;
				
			printf("value = %d\n", value);

			if((flag & QUALITY_ONLINE) != QUALITY_ONLINE)
				item_to_send.iec_obj.o.type37.iv = 1;
		}
		break;
		case EventInterface::AO:
		{
			printf("AO\n");

			item_to_send.iec_obj.ioa = index + OFFSET_AO + 1;

			printf("ioa = %d\n", item_to_send.iec_obj.ioa);

			item_to_send.iec_type = M_ME_TF_1;
			epoch_to_cp56time2a(&time, timestamp);
			item_to_send.iec_obj.o.type36.mv = value;
			item_to_send.iec_obj.o.type36.time = time;

			printf("value = %d\n", value);

			if((flag & QUALITY_ONLINE) != QUALITY_ONLINE)
				item_to_send.iec_obj.o.type36.iv = 1;
		}
		break;
		case EventInterface::BO:
		{
			printf("BO\n");

			item_to_send.iec_obj.ioa = index + OFFSET_BO + 1;

			printf("ioa = %d\n", item_to_send.iec_obj.ioa);

			item_to_send.iec_type = M_SP_TB_1;
			epoch_to_cp56time2a(&time, timestamp);
			item_to_send.iec_obj.o.type30.sp = value;
			item_to_send.iec_obj.o.type30.time = time;

			printf("value = %d\n", value);

			if((flag & QUALITY_ONLINE) != QUALITY_ONLINE)
				item_to_send.iec_obj.o.type30.iv = 1;
		}
		break;
		case EventInterface::NONE:
		{
			printf("NONE\n");
		}
		break;
		case EventInterface::ST:
		{
			printf("ST\n");
		}
		break;
		case EventInterface::AP_AB_ST: // app abnormal stat
		{
			printf("AP_AB_ST\n");
		}
		break;
		case EventInterface::AP_NM_ST: // app normal stat
		{
			printf("AP_NM_ST\n");
		}
		break;
		case EventInterface::DL_AB_ST: // datalink abnormal stat
		{
			printf("DL_AB_ST\n");
		}
		break;
		case EventInterface::DL_NM_ST: // datalink normal stat
		{
			printf("DL_NM_ST\n");
		}
		break;
		case EventInterface::SA_AB_ST: // secure auth abnormal stat
		{
			printf("SA_AB_ST\n");
		}
		break;
		case EventInterface::SA_NM_ST: // secure auth normal stat
		{
			printf("SA_NM_ST\n");
		}
		break;
		case EventInterface::EP_AB_ST: // end point abnormal stat
		{
			printf("EP_AB_ST\n");
		}
		break;
		case EventInterface::EP_NM_ST: // end point normal stat
		{
			printf("EP_NM_ST\n");
		}
		break;
		case EventInterface::NUM_POINT_TYPES:
		{
			printf("NUM_POINT_TYPES\n");
		}
		break;
		default:
			printf("UNSUPPORTED TYPE\n");
		break;
	}

	//IT_COMMENT6("at time: %d_%d_%d_%d_%d_%d", time.hour, time.min, time.msec, time.mday, time.month, time.year);

	item_to_send.msg_id = n_sent_items;
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
	fprintf(stderr,"Sending message %u th\n", n_sent_items);
	fflush(stderr);

	//prepare published data
	memset(global_instanceSend,0x00, sizeof(iec_item_type));
	
	global_instanceSend->iec_type = item_to_send.iec_type;
	memcpy(&(global_instanceSend->iec_obj), &(item_to_send.iec_obj), sizeof(struct iec_object));
	global_instanceSend->cause = item_to_send.cause;
	global_instanceSend->msg_id = item_to_send.msg_id;
	global_instanceSend->ioa_control_center = item_to_send.ioa_control_center;
	global_instanceSend->casdu = item_to_send.casdu;
	global_instanceSend->is_neg = item_to_send.is_neg;
	global_instanceSend->checksum = item_to_send.checksum;

	ORTEPublicationSend(global_publisher);

	n_sent_items++;
}

#include <time.h>
#include <sys/timeb.h>

void CustomDb::epoch_to_cp56time2a(cp56time2a *time, signed __int64 epoch_in_millisec)
{
	struct tm	*ptm;
	int ms = (int)(epoch_in_millisec%1000);
	time_t seconds;
	
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

    return;
}

void  CustomDb::registerName( DnpAddr_t      addr,
			     DnpIndex_t     index,
			     PointType_t    pointType,
			     char*          name,
			     int            initialValue )
{
    assert(addr != 0);
}

// Transmit Interface -----------------

CustomInter::CustomInter(int* debugLevel_p, char name1, char name2, int sck) :
numTxs(0), debug_p(debugLevel_p), socket(NULL)
{
    n[0] = name1;
    n[1] = name2;
    n[2] = 0;
	socket = sck;
}

Uptime_t CustomInter::transmit( const Lpdu& lpdu)
{
    char buf[MAX_LEN*3+1];
    assert(lpdu.ab.size() >= 10);
    lastTxBytes = lpdu.ab;
    if (*debug_p > 0)
	printf( "%s Tx %s\n", n, hex_repr(lpdu.ab, buf,sizeof(buf)));

	char buf_to_send[MAX_LEN*3+1];

	for(unsigned int i = 0; i < lpdu.ab.size(); i++)
    {
        buf_to_send[i] = lpdu.ab[i];
    }

	if(!write(socket, (const char*)buf_to_send, lpdu.ab.size(), 15))
	{
		//Error
		return 1;
	}

    numTxs++;
    return 0;
}

#define WAIT_FOREVER ((time_t)-1)

enum error_codes {
    ok = 0,
    not_opened = -1,
    broken_pipe = -2,
    timeout_expired = -3
};

int CustomInter::read(SOCKET s, void* buf, size_t min_size, size_t max_size, time_t timeout)
{ 
	int errcode = 0;
    size_t size = 0;
    time_t start = 0;

    if (timeout != WAIT_FOREVER) 
	{ 
        start = time(NULL); 
    }

    do{ 
        int rc;

        if (timeout != WAIT_FOREVER)
		{ 
            fd_set events;
            struct timeval tm;
            FD_ZERO(&events);
            FD_SET(s, &events);
            tm.tv_sec = (long)timeout;
            tm.tv_usec = 0;
            rc = select((int)s+1, &events, NULL, NULL, &tm);
            if (rc < 0) 
			{ 
                errcode = WSAGetLastError();
                fprintf(stderr, "Socket select is failed: %d\n", errcode);
			    fflush(stderr);

                return -1;
            }

            if (rc == 0) 
			{
                return size;
            }

            time_t now = time(NULL);
            timeout = start + timeout >= now ? timeout + start - now : 0;  
        }

        rc = recv(s, (char*)buf + size, max_size - size, 0);

        if (rc < 0) 
		{ 
            errcode = WSAGetLastError();
            fprintf(stderr,"Socket read is failed: %d\n", errcode);
			fflush(stderr);

            return -1;
        } 
		else if (rc == 0) 
		{
            errcode = broken_pipe;
            fprintf(stderr,"Socket is disconnected\n");
			fflush(stderr);
            return -1; 
        }
		else 
		{
            size += rc; 
        }

    }while (size < min_size); 

    return (int)size;
}

bool CustomInter::write(SOCKET s, void const* buf, size_t size, time_t timeout)
{ 
	int errcode = 0;
    time_t start = 0;

    if (timeout != WAIT_FOREVER) { 
        start = time(NULL); 
    }
    
    do { 
        int rc;
        if (timeout != WAIT_FOREVER) { 
            fd_set events;
            struct timeval tm;
            FD_ZERO(&events);
            FD_SET(s, &events);
            tm.tv_sec = (long)timeout;
            tm.tv_usec = 0;
            rc = select((int)s+1, NULL, &events, NULL, &tm);
            if (rc <= 0) { 
                errcode = WSAGetLastError();
                fprintf(stderr,"Socket select is failed: %d\n", errcode);
                return false;
            }
            time_t now = time(NULL);
            timeout = start + timeout >= now ? timeout + start - now : 0;  
        }
        rc = send(s, (char*)buf, size, 0);
        if (rc < 0) { 
            errcode = WSAGetLastError();
            fprintf(stderr,"Socket write is failed: %d\n", errcode);
            return false;
        } else if (rc == 0) {
            errcode = broken_pipe;
            fprintf(stderr,"Socket is disconnected\n");
            return false; 
        } else { 
            buf = (char*)buf + rc; 
            size -= rc; 
        }
    } while (size != 0); 

    return true;
}

// Timer Interface --------------------

CustomTimer::CustomTimer() : 
  timerActive(TimerInterface::NUM_TIMERS, false)
{
}

void CustomTimer::activate( TimerId timerId)
{
    timerActive[timerId] = true;
}

void CustomTimer::cancel( TimerId timerId)
{
    timerActive[timerId] = false;
}

bool CustomTimer::isActive( TimerId timerId)
{
    return timerActive[timerId];
}

