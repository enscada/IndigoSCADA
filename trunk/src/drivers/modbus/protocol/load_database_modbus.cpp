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

#ifdef _WIN32

#include <crtdbg.h>
#include <stdio.h>
#include <sqlite3.h>
#include "stdint.h"
#include "iec104types.h"
#include "iec_item.h"
#include "itrace.h"	
#include "modbus_item.h"
#include "modbus_imp.h"
#include "stdlib.h"
#include "string.h"
#endif // _WIN32

static gl_row_counter = 0;
static gl_column_counter = 0;
static struct modbusItem* gl_Config_db = 0;

static int db_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;

	gl_column_counter = argc;
	
	for(i = 0; i < argc; i++)
	{
		fprintf(stderr, "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
		fflush(stderr);

		switch(i)
		{
			case 0:
			{
				//column 1 in table modbus_table
				//name
				if(argv[i] != NULL)
					strcpy(gl_Config_db[gl_row_counter].name, argv[i]);
			}
			break;
			case 1:
			{
				//column 2 in table modbus_table
				//modbus_function_read
				if(argv[i] != NULL)
					gl_Config_db[gl_row_counter].modbus_function_read = atoi(argv[i]);
			}
			break;
			case 2:
			{
				//column 3 in table modbus_table
				//modbus_function_write
				if(argv[i] != NULL)
					gl_Config_db[gl_row_counter].modbus_function_write = atoi(argv[i]);
			}
			break;
			case 3:
			{
				//column 4 in table modbus_table
				//modbus_start_address
				if(argv[i] != NULL)
					gl_Config_db[gl_row_counter].modbus_start_address = atoi(argv[i]);
			}
			break;
			case 4:
			{
				//column 5 in table modbus_table
				//block_size
				if(argv[i] != NULL)
					gl_Config_db[gl_row_counter].block_size = atoi(argv[i]);
			}
			break;
			case 5:
			{
				//column 6 in table modbus_table
				//offset
				if(argv[i] != NULL)
					gl_Config_db[gl_row_counter].offset = atoi(argv[i]);
			}
			break;
			case 6:
			{
				//column 7 in table modbus_table
				//ioa_control_center Unstructured
				if(argv[i] != NULL)
					gl_Config_db[gl_row_counter].ioa_control_center = atoi(argv[i]);
			}
			break;
			case 7:
			{
				//column 8 in table modbus_table
				//iec_type_read
				if(argv[i] != NULL)
				{
					if(strcmp(argv[i], "M_ME_TF_1") == 0)
					{
						gl_Config_db[gl_row_counter].iec_type_read = M_ME_TF_1;
					}
					else if(strcmp(argv[i], "M_SP_TB_1") == 0)
					{
						gl_Config_db[gl_row_counter].iec_type_read = M_SP_TB_1;
					}
					else if(strcmp(argv[i], "M_IT_TB_1") == 0)
					{
						gl_Config_db[gl_row_counter].iec_type_read = M_IT_TB_1;
					}
					else
					{
						fprintf(stderr,"IEC type %s from I/O list NOT supported\n", argv[i]);
						fflush(stderr);
						//ExitProcess(0);
					}
				}
			}	
			break;
			case 8:
			{
				if(argv[i] != NULL)
				{
					//column 9 in table modbus_table
					//iec_type_write
					if(strcmp(argv[i], "C_SC_TA_1") == 0)
					{
						gl_Config_db[gl_row_counter].iec_type_write = C_SC_TA_1;
					}
					else if(strcmp(argv[i], "C_BO_TA_1") == 0)
					{
						gl_Config_db[gl_row_counter].iec_type_write = C_BO_TA_1;
					}
					else if(strcmp(argv[i], "C_SE_TC_1") == 0)
					{
						gl_Config_db[gl_row_counter].iec_type_write = C_SE_TC_1;
					}
					else
					{
						fprintf(stderr,"IEC type %s from I/O list NOT supported\n", argv[i]);
						fflush(stderr);
						//ExitProcess(0);
					}
				}
			}	
			break;
			case 9:
			{
				//column 10 in table modbus_table
				//size_in_bits_of_iec_type
				if(argv[i] != NULL)
					gl_Config_db[gl_row_counter].size_in_bits_of_iec_type = atoi(argv[i]);
			}
			break;
			default:
			break;
		}
	}

	//ended to read a record
	gl_row_counter++;

	fprintf(stderr, "\n");
	fflush(stderr);
	return 0;
}

#define MAX_CONFIGURABLE_MODBUS_ITEMIDS 30000

int modbus_imp::AddItems(void)
{
	IT_IT("modbus_imp::AddItems");

	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	//FILE* fp = NULL;

	strcpy(database_name, "C:\\scada\\bin\\");
	strcat(database_name, "modbus_database");
	strcat(database_name, lineNumber);
	strcat(database_name, ".db");

	rc = sqlite3_open(database_name, &db);

	if(rc)
	{
	  fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
	  fflush(stderr);
	  sqlite3_close(db);
	  IT_EXIT;
	  return 1;
	}

	g_dwNumItems = MAX_CONFIGURABLE_MODBUS_ITEMIDS;
	
	Config_db = (struct modbusItem*)calloc(1, g_dwNumItems*sizeof(struct modbusItem));

	gl_Config_db = Config_db;

	gl_row_counter = 0;

	rc = sqlite3_exec(db, "select * from modbus_table;", db_callback, 0, &zErrMsg);

	if(rc != SQLITE_OK)
	{
	  fprintf(stderr, "SQL error: %s\n", zErrMsg);
	  fflush(stderr);
	  sqlite3_free(zErrMsg);
	}

	sqlite3_close(db);

	db_n_rows = gl_row_counter;
	db_m_columns = gl_column_counter;

	if(db_n_rows == 0)
	{
		fprintf(stderr, "Error: db_n_rows = %d\n", db_n_rows);
		fflush(stderr);
		IT_EXIT;
		return 1;
	}
	
	g_dwNumItems = db_n_rows;


	return(0);
}
