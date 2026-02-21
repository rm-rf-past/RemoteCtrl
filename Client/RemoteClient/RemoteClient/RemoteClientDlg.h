
// RemoteClientDlg.h: 头文件
//

#pragma once


// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedTestButton();
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnEnChangeIp();
	DWORD m_server_address;
	CString m_port;
private:
	//返回值：是命令号，如果小于0则是错误
	// 客户端快捷建立连接并发送消息的接口
	int sendCommandPacket(int nCmd, BOOL bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);
	void LoadFileInfo();
	void DeleteTreeChildrenItem(HTREEITEM h_tree);
	CString CRemoteClientDlg::GetPath(HTREEITEM hTree);
public:
	afx_msg void OnBnClickedShowFile();
	CTreeCtrl m_tree;
	afx_msg void OnNMDblclkTreeFile(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_list;
};
