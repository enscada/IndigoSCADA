/**********************************************************************
--- Qt Architect generated file ---
File: CalculatedConfigureData.h
Last generated: Thu Jan 4 16:11:31 2001
DO NOT EDIT!!!  This file will be automatically
regenerated by qtarch.  All changes will be lost.
*********************************************************************/
#ifndef CalculatedConfigureData_included
#define CalculatedConfigureData_included
#include <qdialog.h>
#include <qlabel.h>
#include <qspinbox.h>
class CalculatedConfigureData : public QDialog
{
	Q_OBJECT
	public:
	CalculatedConfigureData(QWidget *parent = NULL, const char *name = NULL);
	virtual ~CalculatedConfigureData();
	protected slots:
	virtual void Help() =0;
	virtual void OkClicked() =0;
	protected:
	QLabel *Name;
	QSpinBox *ReCalculateInterval;
};
#endif // CalculatedConfigureData_included

