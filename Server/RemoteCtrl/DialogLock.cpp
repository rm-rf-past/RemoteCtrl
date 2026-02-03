// DialogLock.cpp: 实现文件
//

#include "pch.h"
#include "RemoteCtrl.h"
#include "afxdialogex.h"
#include "DialogLock.h"


// CDialogLock 对话框

IMPLEMENT_DYNAMIC(CDialogLock, CDialog)

CDialogLock::CDialogLock(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DIALOG_LOCK, pParent)
{

}

CDialogLock::~CDialogLock()
{
}

void CDialogLock::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}
BEGIN_MESSAGE_MAP(CDialogLock, CDialog)

END_MESSAGE_MAP()