/**********************************************************************
--- Qt Architect generated file ---
File: SystemConfigureData.h
Last generated: Thu Jan 4 16:20:37 2001
DO NOT EDIT!!!  This file will be automatically
regenerated by qtarch.  All changes will be lost.
*********************************************************************/
#ifndef SystemConfigureData_included
#define SystemConfigureData_included
#include <qdialog.h>
#include <qcombobox.h>
#include <qspinbox.h>
class SystemConfigureData : public QDialog
{
	Q_OBJECT
	public:
	SystemConfigureData(QWidget *parent = NULL, const char *name = NULL);
	virtual ~SystemConfigureData();
	protected slots:
	virtual void Help() =0;
	virtual void OkClicked() =0;
	protected:
	QSpinBox *MinDiskSpace;
	QComboBox *Drive;
};
#endif // SystemConfigureData_included
