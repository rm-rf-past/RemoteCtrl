// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>  //用于支持文件命令
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

// 检查磁盘信息
std::string makeDriverInfo() {
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
    std::cout << "sssssss";
    return result;
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
            case 1:
                makeDriverInfo();
                break;
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
            //    //p_server->dealCommand();
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
