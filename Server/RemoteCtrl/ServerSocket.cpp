#include "pch.h"
#include"ServerSocket.h"

//CServerSocket server;  //全局变量。在main函数之前执行构造，单线程。
//上述使用会有潜在风险。同事使用这个类创建局部对象，可能会导致socket环境提前析构

CServerSocket* CServerSocket::m_instance = NULL;  //标准初始化方式，但是可以更简洁
CServerSocket::CHelper CServerSocket::m_helper;   // 为什么私有类能够被构造？这是一个C++的特殊规则：类的静态成员定义可以访问私有类型。
