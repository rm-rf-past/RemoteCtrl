// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>  //用于支持文件命令
#include <io.h>  //用于支持_findfirst()方法
#include <list>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;


// 检查磁盘信息
int makeDriverInfo() {
    std::string result;
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {
            if (i != 1) {
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
    }
    if (_chdir(str_file_path.c_str()) != 0) {
        FILEINFO file_info;
        file_info.isInvalid = true;
        file_info.isDirectory = true;
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
        return -3;
    }

    do {
        FILEINFO file_info;
        file_info.isDirectory = (fdata.attrib & _A_SUBDIR) !=0;  //该属性指明是否是目录
        memcpy(&file_info.str_filename, fdata.name, strlen(fdata.name));
        CPacket packet(2, (const char*)&file_info, sizeof(file_info));
        CServerSocket::getInstance()->sendByte(packet);
    } while (!_findnext(hfind, &fdata));
    // 处理最后一个没有发出去的文件
    FILEINFO file_info;
    file_info.hasNext = false;
    CPacket packet(2, (const char*)&file_info, sizeof(file_info));
    CServerSocket::getInstance()->sendByte(packet);

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
        CPacket packet(4, (const char*)&data, 8);  //告知长度为0
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
    CPacket packet(4, NULL, 0); //不清楚为什么这里要发个0包。视频可能为了防止文件传一半失败/文件本身为空，相当于一个结束符。
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
            uint32_t command = 1;
            switch (command) {
            case 1:  //获取分区信息
                makeDriverInfo();
                break;
            case 2:  //获取指定目录的文件信息
                makeDirectoryInfo();
                break;
            case 3:  // 打开指定的文件
                runFile();
                break;
            case 4:  //下载文件
                download_file();
                break;
            case 5: //鼠标操作
                mouseEvent();
            }
            // TODO: 在此处为应用程序的行为编写代码。
            //CServerSocket* p_server = CServerSocket::getInstance();
            //int count = 0;
            //while (p_server) {
            //    if (p_server->initSocket() == false) {
            //        MessageBox(NULL, _T("socket初始化失败"), _T("错误"), MB_OK | MB_ICONERROR);
            //        exit(0);
            //    }
            //    if (p_server->acceptClient() == false) {
            //        if (count >= 3) {
            //            MessageBox(NULL, _T("多次无法接入用户，结束程序"), _T("错误"), MB_OK | MB_ICONERROR);
            //            exit(0);
            //        }
            //        MessageBox(NULL, _T("接入用户失败"), _T("错误"), MB_OK | MB_ICONERROR);
            //        count++;
            //    }
            //    //成功初始化和接入
                //p_server->dealCommand();
            //}
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
