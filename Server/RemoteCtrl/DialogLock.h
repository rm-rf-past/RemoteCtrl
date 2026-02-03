#pragma once
#include "afxdialogex.h"


// CDialogLock 对话框

class CDialogLock : public CDialog
{
	DECLARE_DYNAMIC(CDialogLock)

public:
	CDialogLock(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CDialogLock();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_LOCK };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
};
