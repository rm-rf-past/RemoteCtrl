
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientSocket.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_port(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IP, m_server_address);
	DDX_Text(pDX, IDC_PORT, m_port);
	DDX_Control(pDX, IDC_TREE_FILE, m_tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_list);
}

BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(ID_TEST_BUTTON, &CRemoteClientDlg::OnBnClickedTestButton)
	ON_BN_CLICKED(IDC_SHOW_FILE, &CRemoteClientDlg::OnBnClickedShowFile)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_FILE, &CRemoteClientDlg::OnNMDblclkTreeFile)
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData();
	m_server_address = 0x7F000001;
	m_port = "1234";
	UpdateData(false);
	m_tree.ModifyStyle(0, TVS_HASLINES | TVS_SHOWSELALWAYS | TVS_LINESATROOT | TVS_NOSCROLL | TVS_TRACKSELECT | TVS_HASBUTTONS); // | TVS_HASBUTTONS | TVS_SINGLEEXPAND 

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CRemoteClientDlg::OnBnClickedTestButton()
{
	// TODO: 在此添加控件通知处理程序代码
	sendCommandPacket(9999);
}

//1 查看磁盘分区
//2 查看指定目录下的文件
//3 打开文件
//4 下载文件
//9 删除文件
//5 鼠标操作
//6 发送屏幕内容
//7 锁机
//8 解锁
//1981 测试连接
int CRemoteClientDlg::sendCommandPacket(int nCmd, BOOL bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, _ttoi(m_port));
	if (!ret) {
		AfxMessageBox(_T("网络初始化失败!"));
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);
	TRACE("Send ret %d\r\n", ret);
	int cmd = pClient->DealCommand();   //客户端这里能够deal命令，是因为服务端接收每一条命令，都会返回相同命令号的空包
	TRACE("ack:%d\r\n", cmd);
	if (bAutoClose)   // 在获取多条文件信息的时候，连接不应该关闭，所以autoClose来控制
		pClient->CloseSocket();
	return cmd;
}

void CRemoteClientDlg::OnBnClickedShowFile()
{
	// TODO: 在此添加控件通知处理程序代码
	int ret = sendCommandPacket(1);   // 1号命令：获得逗号分隔的盘符
	if (ret == -1) {
		AfxMessageBox(_T("命令处理失败!!!"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers = pClient->GetPacket().strData;   //获取包的body
	std::string dr;
	m_tree.DeleteAllItems();  //清空树控件
	for (size_t i = 0; i < drivers.size(); i++)
	{
		if (drivers[i] == ',') {
			dr += ":";
			HTREEITEM hTemp = m_tree.InsertItem(CString(dr.c_str()), TVI_ROOT, TVI_LAST);  // 在树节点的尾部，不断插入分区
			m_tree.InsertItem(NULL, hTemp, TVI_LAST);  // 给分区插入一个空孩子，表示这是一个目录
			dr.clear();
			continue;
		}
		dr += drivers[i];
	}
}

void CRemoteClientDlg::LoadFileInfo()
{
	CPoint ptMouse;
	GetCursorPos(&ptMouse);   // 获取鼠标位置，这是全局坐标
	m_tree.ScreenToClient(&ptMouse);  // 转换全局坐标为对话框相对坐标
	HTREEITEM hTreeSelected = m_tree.HitTest(ptMouse, 0);   // 坐标检测，检查ptMouse是否在树控件范围
	if (hTreeSelected == NULL)  // 处理在树控件范围内，什么都没点到的情况（即no item selected）
		return;
	if (m_tree.GetChildItem(hTreeSelected) == NULL)   // 点击的节点没有孩子节点（注意，所有的目录文件都至少有一个空孩子），也不需要展开
		return;
	// 需要展开选中节点的情况
	// 首先关闭其他可能展开的节点
	DeleteTreeChildrenItem(hTreeSelected);   // 实现方式：删除所有子节点
	CString strPath = GetPath(hTreeSelected);    // 获取点击节点的路径
	TRACE("===================%s", strPath);
	int nCmd = sendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());   // 根据路径查询节点目录下的文件
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();  // 拿到第一个文件结构体
	TRACE("===================%s", pInfo->szFileName);
	CClientSocket* pClient = CClientSocket::getInstance();
	int Count = 0;
	while (pInfo->HasNext) {
		TRACE("[%s] isdir %d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {
			if (CString(pInfo->szFileName) == "." || (CString(pInfo->szFileName) == ".."))  // 对于双击两个特殊符号，应该不做处理，继续循环
			{
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0)break;
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_tree.InsertItem(CString(pInfo->szFileName), hTreeSelected, TVI_LAST);
			m_tree.InsertItem(CString(""), hTemp, TVI_LAST);
		}
		Count++;
		int cmd = pClient->DealCommand();
		//TRACE("ack:%d\r\n", cmd);
		if (cmd < 0)break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
	TRACE("Count = %d\r\n", Count);
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM h_tree)
{
	HTREEITEM hSub = NULL;
	do {
		hSub = m_tree.GetChildItem(h_tree);
		if (hSub != NULL)m_tree.DeleteItem(hSub);
	} while (hSub != NULL);
}


CString CRemoteClientDlg::GetPath(HTREEITEM hTree)
{
	CString strRet, strTmp;
	do {
		strTmp = m_tree.GetItemText(hTree);
		strRet = strTmp + '\\' + strRet;
		hTree = m_tree.GetParentItem(hTree);
	} while (hTree != NULL);   // 向上迭代，例如展开为C:/a/b/c/。那么要获取c所在的目录，需要每次添加自己的父母str_tmp
	return strRet;
}

void CRemoteClientDlg::OnNMDblclkTreeFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}
