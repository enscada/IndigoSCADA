/**********************************************************************
--- Qt Architect generated file ---
File: LoginDlgData.h
Last generated: Fri Feb 16 15:10:17 2001
DO NOT EDIT!!!  This file will be automatically
regenerated by qtarch.  All changes will be lost.
*********************************************************************/
#ifndef LoginDlgData_included
#define LoginDlgData_included
#include <qt.h>

class LoginDlgData : public QDialog
{
	Q_OBJECT
	public:
	LoginDlgData(QWidget *parent = NULL, const char *name = NULL);
	virtual ~LoginDlgData();
	protected slots:
	virtual void NameChanged(const QString&) =0;
	virtual void Okclicked() =0;
	protected:
	QPushButton *OkButton;
	QLineEdit *Username;
	QLineEdit *Password;
};
#endif // LoginDlgData_included

