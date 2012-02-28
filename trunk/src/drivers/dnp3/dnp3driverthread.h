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
#ifndef DNP3_DRIVERTHREAD
#define DNP3_DRIVERTHREAD

#include "general_defines.h"
#include "IndentedTrace.h"
#include "dnp3driver_instance.h"

const int nMaxProcCount = 127;
const int nBufferSize = 500;

class DNP_3_DRIVERDRV Dnp3DriverThread : public DriverThread 
{
	public:

    ///////////////Child process support/////////////
    PROCESS_INFORMATION pProcInfo[nMaxProcCount];
    char pCommandLine[nBufferSize+1];
    char pWorkingDir[nBufferSize+1];
    int nIndex;
	char pipe_name[150];
    HANDLE h_pipe;
    int pipe_sends_cont;
    int msg_id;
    int instanceID;
    bool Done;
    /////////////////////////////////////////////////

	Dnp3DriverThread(DriverInstance *parent) : 
		DriverThread(parent),h_pipe(NULL), nIndex(1), pipe_sends_cont(0),
        msg_id(0), Done(false)
	{ 
		IT_IT("Dnp3DriverThread::Dnp3DriverThread");

		instanceID = ((Dnp3driver_Instance*)Parent)->instanceID;

        ///////////////Child process support/////////////
        for(int i = 0; i < nMaxProcCount; i++)
        {
            memset(pProcInfo + i, 0x00, sizeof(PROCESS_INFORMATION));
        }
        /////////////////////////////////////////////////
	};
	
	~Dnp3DriverThread()
	{
		IT_IT("Dnp3DriverThread::~Dnp3DriverThread");
	}

    void TerminateIEC(); // parent requests the thread close
    //PROCESS_INFORMATION* getProcInfo(void);

	protected:
	void run(); // thread main routine
    ///////////////Child process support/////////////////////////////////////////////////////
    bool StartProcess(char* pCommandLine, char* pWorkingDir);
    void EndProcess(int nIndex);
    int send_ack_to_child(int address, int data, char* pipeName);
    int pipe_put(char* pipe_name, char *buf, int len);
	/////////////////////////////////////////////////////////////////////////////////////////
};

#endif //DNP3_DRIVERTHREAD