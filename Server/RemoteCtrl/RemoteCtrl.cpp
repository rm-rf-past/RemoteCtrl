// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>  //用于支持文件命令
#include <io.h>  //用于支持_findfirst()方法
#include <list>
#include <atlimage.h>
#include "DialogLock.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

// 唯一锁定对话框

CDialogLock dlg;

uint32_t thread_id;  //锁机线程

using namespace std;


// 检查磁盘信息
int makeDriverInfo() {
    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {
            if (result.size()!=0) {
                result += ",";  // 不是第一个盘需要加分隔符

            }
            result += 'A' + i - 1;
        }
    }
    //CPacket((WORD)(1), result.data(), result.size());
    CPacket packet((WORD)(1), result.data(), result.size());
    CServerSocket::getInstance()->sendByte(packet);
    return 0;
}

// 查找指定目录的文件
int makeDirectoryInfo() {
    std::string str_file_path;
    if (CServerSocket::getInstance()->getFilePath(str_file_path) == false) {
        OutputDebugString(_T("当前的命令不是获取文件列表，命令解析错误"));
        return -1;
    }
    if (_chdir(str_file_path.c_str()) != 0) {
        FILEINFO file_info;
        file_info.hasNext = false;
        CPacket packet( 2, (const char*) &file_info, sizeof(file_info));  //找到一个直接发，而不是找到全部再发，用户体验更好
        CServerSocket::getInstance()->sendByte(packet);
        OutputDebugString(_T("缺少访问目标路径的权限"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind = _findfirst("*", &fdata)) == -1) {
        OutputDebugString(_T("没有找到任何文件"));
        FILEINFO finfo;
        finfo.hasNext = FALSE;
        CPacket pack(2, (const char*)&finfo, sizeof(finfo));
        CServerSocket::getInstance()->sendByte(pack);
        return -3;
    }

    int count = 0;
    do {
        FILEINFO file_info;
        file_info.isDirectory = (fdata.attrib & _A_SUBDIR) !=0;  //该属性指明是否是目录
        memcpy(&file_info.str_filename, fdata.name, strlen(fdata.name));
        CPacket packet(2, (const char*)&file_info, sizeof(file_info));
        CServerSocket::getInstance()->sendByte(packet);
    } while (!_findnext(hfind, &fdata));
    TRACE("server: count = %d\r\n", count);
    // 处理最后一个没有发出去的文件
    FILEINFO file_info;
    file_info.hasNext = false;
    CPacket packet(2, (const char*)&file_info, sizeof(file_info));
    CServerSocket::getInstance()->sendByte(packet);
    return 0;
}

// 打开文件
int runFile() {
    std::string str_file_path;
    CServerSocket::getInstance()->getFilePath(str_file_path);
    ShellExecuteA(NULL, NULL, NULL, str_file_path.c_str(), NULL,SW_NORMAL);  //等同于双击打开了文件，exe/txt都会有默认的执行方式
    CPacket packet(3, NULL, 0);
    return 0;
}

// 下载文件
int download_file() {
    std::string str_file_path;
    long long data = 0;      //始终先传递文件大小（方便实现进度条等功能）
    CServerSocket::getInstance()->getFilePath(str_file_path);
    FILE* p_file = NULL;
    errno_t err = fopen_s(&p_file,str_file_path.c_str(), "rb");  // fopen_s给文件打开到一半又失败的情况打补丁（常见于多进程环境）
    if (err != -1) {
        CPacket packet(4, (const char*)&data, 8);  //告知长度为0，相当于结束符
        CServerSocket::getInstance()->sendByte(packet);
        return -1;
    }

    // 打开成功
    if (p_file != NULL) {
        fseek(p_file, 0, SEEK_END);
        data = _ftelli64(p_file);
        CPacket head(4, (const char*)&data, 8);
        CServerSocket::getInstance()->sendByte(head);
        fseek(p_file, 0, SEEK_SET);  //把文件指针还原到头
        char buffer[1024];
        uint32_t read_length;
        do {
            read_length = fread(buffer, 1, 1024, p_file);
            CPacket packet(4, buffer, read_length);
            CServerSocket::getInstance()->sendByte(packet);
        } while (read_length >= 1024);  //小于1024说明是文件末尾，恰好退出
        fclose(p_file);
    }
    CPacket packet(4, NULL, 0); //视频可能为了防止文件传一半失败/文件本身为空，相当于一个结束符。
    CServerSocket::getInstance()->sendByte(packet);

    return 0;
}

// 鼠标操作
int mouseEvent() {
    MOUSEEVENT mouse;
    if (CServerSocket::getInstance()->getMouseEvent(mouse)) {
        DWORD flags = 0;
        switch (mouse.button) {
        case 0: //左键
            flags = 1;
            break;
        case 1: //右键
            flags = 2;
            break;
        case 2: //中键
            flags = 4;
            break;
        case 3: //纯移动，无按键
            flags = 8;
        }
        if(flags!=8){ SetCursorPos(mouse.pt.x, mouse.pt.y); }  //没有按键就不执行设置鼠标位置了。
        switch (mouse.action) {
        case 0: //单击
            flags |= 0x10;
            break;
        case 1: //双击
            flags |= 0x20;
            break;
        case 2: //按下
            flags |= 0x40;
            break;
        case 3: //放开
            flags |= 0x80;
            break;
        default: break;  // 处理无按键的情况
        }

        switch (flags) {

        case 0x11: // 左键单击 (Left Click: 1 | 0x10)
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x21: // 左键双击 (Left Double Click: 1 | 0x20)
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x41: // 左键按下 (Left Down: 1 | 0x40)
            mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x81: // 左键放开 (Left Up: 1 | 0x80)
            mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x12: // 右键单击 (Right Click: 2 | 0x10)
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x22: // 右键双击 (Right Double Click: 2 | 0x20)
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x42: // 右键按下 (Right Down: 2 | 0x40)
            mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x82: // 右键放开 (Right Up: 2 | 0x80)
            mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x14: // 中键单击 (Middle Click: 4 | 0x10)
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x24: // 中键双击 (Middle Double Click: 4 | 0x20)
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x44: // 中键按下 (Middle Down: 4 | 0x40)
            mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
            break;
        case 0x84: // 中键放开 (Middle Up: 4 | 0x80)
            mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
            break;

        case 0x08: // 纯移动 (Move: 8)
            mouse_event(MOUSEEVENTF_MOVE, mouse.pt.x, mouse.pt.y, 0, GetMessageExtraInfo());
            break;

        default:
            break;
        }

        CPacket packet(4, NULL, 0);
        CServerSocket::getInstance()->getInstance()->sendByte(packet);

    }
    else {
        OutputDebugString(_T("获取鼠标操作参数失败"));
        return -1;
    }
}

int sendScreen() {
    CImage screen;  //Global Devive Interface GDI
    HDC handle_screen = ::GetDC(NULL);   // 屏幕句柄
    int bit_per_pixel = GetDeviceCaps(handle_screen, BITSPIXEL);  // 获取颜色位宽
    int screen_width = GetDeviceCaps(handle_screen, HORZRES);   // 获取屏幕宽度
    int screen_height = GetDeviceCaps(handle_screen, VERTRES);  // 获取屏幕高度
    screen.Create(screen_width, screen_height, bit_per_pixel);  //创建IMAGE空对象
    BitBlt(screen.GetDC(), 0, 0, screen_width, screen_height, handle_screen, 0, 0, SRCCOPY);  // 拷贝屏幕内容到IMAGE对象
    ReleaseDC(NULL, handle_screen);
    // 测试两种格式的保存时间和大小
    //for (int i = 0; i < 10; i++) {
    //    DWORD tick = GetTickCount64();
    //    screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);
    //    TRACE("png: %d\r\n", GetTickCount64() - tick);
    //    tick = GetTickCount64();
    //    screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);
    //    TRACE("jpeg: %d\r\n", GetTickCount64() - tick);
    //}
    //screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);  //显然不应该写入磁盘，而应该建立内存对象直接发送
    HGLOBAL handle_memory =  GlobalAlloc(GMEM_MOVEABLE, 0);  
    IStream* p_stream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(handle_memory, TRUE, &p_stream);   // 在全局对象创建流，这个流会自动扩容
    if (ret == S_OK) {
        screen.Save(p_stream, Gdiplus::ImageFormatPNG);   // 图片数据->流对象
        LARGE_INTEGER begin = {0};
        SIZE_T size = GlobalSize(handle_memory);  // 保存当前流对象的大小
        p_stream->Seek(begin, STREAM_SEEK_SET, NULL);
        PBYTE p_data = (PBYTE)GlobalLock(handle_memory);
        CPacket packet(6, (const char*)p_data, size);    // 构造包
        CServerSocket::getInstance()->sendByte(packet);
        GlobalUnlock(handle_memory);  // lock和unlock成对使用
    }
    p_stream->Release();
    GlobalFree(handle_memory);
    screen.ReleaseDC();
    return 0;
}


unsigned int __stdcall threadLockDialog(void* arg) {
    TRACE("%s(%d):%d\r\n", __FUNCTION__, __LINE__, GetCurrentThreadId());
    // 获取屏幕分辨率
    dlg.Create(IDD_DIALOG_LOCK, NULL);  // 这一行会绑定dlg对象和资源IDD_DIALOG_LOCK，并向OS请求和保存窗口句柄
    int cx = GetSystemMetrics(SM_CXSCREEN);
    int cy = GetSystemMetrics(SM_CYSCREEN);

    // 去除标题框等内容
    dlg.ModifyStyle(WS_CAPTION | WS_THICKFRAME | WS_SYSMENU, 0);
    dlg.SetWindowPos(&CWnd::wndTopMost, 0, 0, cx, cy, SWP_SHOWWINDOW);

    // 设置文本居中
    CWnd* p_text = dlg.GetDlgItem(ID_TEXT);  //文本句柄
    CWnd* p_taskbar = CWnd::FindWindow(_T("Shell_TrayWnd"), NULL); //任务栏句柄
    if (p_taskbar) {
        p_taskbar->ShowWindow(SW_HIDE);
    }
    if (p_text != NULL) {
        // A. 获取整个屏幕（对话框客户区）的大小
        CRect rectClient;
        dlg.GetClientRect(&rectClient);
        // B. 获取文本控件原本的大小
        CRect rectStatic;
        p_text->GetWindowRect(&rectStatic);
        int w = rectStatic.Width();
        int h = rectStatic.Height();
        // C. 计算居中的左上角坐标 (X, Y)
        // 公式：(屏幕宽 - 控件宽) / 2
        int x = (rectClient.Width() - w) / 2;
        int y = (rectClient.Height() - h) / 2;
        // D. 移动控件到计算好的位置
        p_text->SetWindowPos(NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    }

    // 显示对话框
    dlg.ShowWindow(SW_SHOW);

    // 隐藏鼠标并限制鼠标到位置(1,1)
    ShowCursor(false);
    CRect rec(0, 0, 1, 1);
    //dlg.GetWindowRect(rec);
    ClipCursor(rec);

    //消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_KEYDOWN) {
            TRACE("msg:%08X wparam:%08x lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);
            if (msg.wParam == VK_ESCAPE) {//按下ESC退出
                break;
            }
        }
    }
    ClipCursor(NULL);
    dlg.DestroyWindow();  //销毁窗口会使得m_hWnd为空
    //恢复鼠标和任务栏
    ShowCursor(true);
    if (p_taskbar) {
        p_taskbar->ShowWindow(SW_SHOW);
    }
    _endthreadex(0);
    return 0;
}

int lockDevice() {
    // 检查当前未锁定，才创建锁定窗口线程
    if (dlg.m_hWnd == NULL || dlg.m_hWnd == INVALID_HANDLE_VALUE) {
        // 这里要开线程来锁定，因为主线程在锁定期间，还需要接收远程控制消息
        _beginthreadex(NULL, 0, threadLockDialog, NULL, 0, &thread_id);
        TRACE("threadid=%d\r\n", thread_id);
    }
    CPacket pack(7, NULL, 0);
    CServerSocket::getInstance()->sendByte(pack);
    return 0;
}

int unlockDevice() {
    //注意，消息分发是会分线程来的。所以dlg.SendMessage以及::SendMessage，都无法将消息发到子线程
    PostThreadMessage(thread_id , WM_KEYDOWN, VK_ESCAPE, 0);  //向锁机线程发送消息
    CPacket pack(8, NULL, 0);
    CServerSocket::getInstance()->sendByte(pack);
    return 0;
}

int  testConnect() {
    CPacket packet(9999, NULL, 0);
    CServerSocket::getInstance()->sendByte(packet);  // 收到9999命令，ack一个9999的空包
    return 0;
}

int executeCommand(int command) {
    int ret = 0;
    switch (command) {
    case 1:  //获取分区信息
        ret = makeDriverInfo();
        break;
    case 2:  //获取指定目录的文件信息
        ret = makeDirectoryInfo();
        break;
    case 3:  // 打开指定的文件
        ret = runFile();
        break;
    case 4:  //下载文件
        ret = download_file();
        break;
    case 5: //鼠标操作
        ret = mouseEvent();
        break;
    case 6: //发送屏幕截图
        ret = sendScreen();
        break;
    case 7:  // 锁机
        ret = lockDevice();
        break;
    case 8:  //解锁
        unlockDevice();
        break;
    case 9999: //连接测试
        ret = testConnect();
    }
    return ret;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
             //TODO: 在此处为应用程序的行为编写代码。
            CServerSocket* p_server = CServerSocket::getInstance();
            // 初始化server socket
            if (p_server->initSocket() == false) {
                MessageBox(NULL, _T("socket初始化失败1"), _T("错误"), MB_OK | MB_ICONERROR);
                exit(0);
            }
            int count = 0;
            while (p_server) {
                // 阻塞等待用户连接
                if (p_server->acceptClient() == false) {
                    if (count >= 3) {
                        MessageBox(NULL, _T("多次无法接入用户，结束程序"), _T("错误"), MB_OK | MB_ICONERROR);
                        exit(0);
                    }
                    MessageBox(NULL, _T("接入用户失败"), _T("错误"), MB_OK | MB_ICONERROR);
                    count++;
                }

                TRACE("客户端接入成功");
                // 解析用户命令
                int ret = p_server->dealCommand();
                TRACE("deal command ret = ", ret);
                // 执行用户命令
                if (ret != -1){
                    ret = executeCommand(ret);
                    if (ret != 0) {
                        TRACE("执行命令失败：%d ret=%d\r\n", p_server->GetPacket().s_command, ret);
                    }
                    
                    p_server->closeClient();
                }
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
