/**********************************************************************
--- Qt Architect generated file ---
File: Opc_client_aeConfigurationData.h
Last generated: Thu Jan 4 16:13:32 2001
DO NOT EDIT!!!  This file will be automatically
regenerated by qtarch.  All changes will be lost.
*********************************************************************/
#ifndef Opc_client_aeConfigurationData_included
#define Opc_client_aeConfigurationData_included

#include <qt.h>

class Opc_client_aeConfigurationData : public QDialog
{
	Q_OBJECT
	public:
	Opc_client_aeConfigurationData(QWidget *parent = NULL, const char *name = NULL);
	virtual ~Opc_client_aeConfigurationData();
	protected slots:
	virtual void Help() =0;
	virtual void OkClicked() =0;
	protected:
	QLabel *Name;
	QSpinBox *NItems;
	QSpinBox *PollInterval;
	QLineEdit *OpcServerProgIDText;
	QLineEdit *OpcServerIPAddressText;
	QLineEdit *OpcServerClassIDText;
};
#endif // Opc_client_aeConfigurationData_included

