#pragma once
#include "pch.h"
#define BUFFER_SIZE 4096
#include <iomanip> // 必须引入这个头文件用于 hex, setw
#include <windows.h> // OutputDebugStringA 需要此头文件
#include <stdio.h>   // snprintf 需要此头文件

// 定义文件信息
typedef struct file_info {
	file_info() {
		isInvalid = false;
		isDirectory = -1;
		hasNext = true;
		memset(str_filename, 0, sizeof(str_filename));
	}
	BOOL isInvalid;
	BOOL isDirectory;
	BOOL hasNext;  //解决文件极多，用户得不到反馈的情况。
	char str_filename[256];


}FILEINFO, * PFILEINFO;
// 定义鼠标事件
typedef struct mouse_event {
	mouse_event() {
		action = 0;
		button = -1;  //0左，1右，2中
		pt.x = 0;
		pt.y = 0;
	}
	WORD action; //点击，移动，双击
	WORD button; //左键，右键，中键
	POINT pt;  //坐标，包含long X/Y
}MOUSEEVENT, * PMOUSEEVENT;

#pragma pack(push)
#pragma pack(1)
class CPacket {

public:
	WORD s_magic;  //固定为FE FF
	DWORD length;
	// 以下为length长度的计算范围
	WORD s_command;
	std::string str_data;
	WORD s_sum;
	std::string m_raw_data;

	// 无参构造
	CPacket() :s_magic(0), length(0), s_command(0), s_sum(0) {}

	// 拷贝构造
	CPacket(const CPacket& rhs) {
		s_magic = rhs.s_magic;
		length = rhs.length;
		s_command = rhs.s_command;
		str_data = rhs.str_data;
		s_sum = rhs.s_sum;
	}

	// 赋值运算符
	CPacket & operator=(const CPacket& rhs) {
		if (this != &rhs) {
			s_magic = rhs.s_magic;
			length = rhs.length;
			s_command = rhs.s_command;
			str_data = rhs.str_data;
			s_sum = rhs.s_sum;
		}
		return *this;

	}

	// 打包构造。常用于发送命令和数据
	CPacket(WORD command, const char* data,uint32_t size) {
		s_magic = 0xFEFF;
		length = size + sizeof(s_command) + sizeof(s_sum);  // 命令+数据+校验和
		s_command = command;
		str_data.resize(size);
		memcpy(str_data.data(), data, size);
		s_sum = 0;   //注意，校验只针对str_data部分进行，而不是整个包
		for (int i = 0; i < str_data.size(); i++) {
			s_sum += (BYTE)str_data[i] & 0xFF;
		}
	}

	// 解包构造
	// 解析输入：缓冲区数据data，以及data长度size
	// 解析输出：size>0表示解析成功
	// size表示解析出的整个包的长度，length表示body长度
	CPacket(const BYTE* data, uint32_t &size) {
		size_t i = 0;
		for (; i < size; i++) {
			if (*(WORD*)(data + i) == 0xFEFF) {
				s_magic = *(WORD*)(data + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > size) {//包数据可能不全，或者包头未能全部接收到
			size = 0;
			return;
		}
		length = *(DWORD*)(data + i); i += 4;
		if (length + i > size) {//包未完全接收到，就返回，解析失败
			size = 0;
			return;
		}
		s_command = *(WORD*)(data + i); i += 2;
		if (length > 4) {
			str_data.resize(length - 2 - 2);
			memcpy((void*)str_data.c_str(), data + i, length - 4);
			i += length - 4;
		}
		s_sum = *(WORD*)(data + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < str_data.size(); j++)
		{
			sum += BYTE(str_data[j]) & 0xFF;
		}
		if (sum == s_sum) {
			size = i;//head2 length4 data...
			return;
		}
		size = 0;
	}

	// 序列化方法，返回字节
	const char* data() {
		m_raw_data.resize(length + sizeof(s_magic) + sizeof(length));
		BYTE* offset = (BYTE*)m_raw_data.data();
		memcpy(offset, &s_magic, sizeof(s_magic)); //拷贝魔数
		offset += sizeof(s_magic);
		memcpy(offset, &length, sizeof(length)); //拷贝长度
		offset += sizeof(length);
		memcpy(offset, &s_command, sizeof(s_command)); //拷贝命令
		offset += sizeof(s_command);
		memcpy(offset, str_data.data(), str_data.size()); //拷贝数据
		offset += str_data.size();
		memcpy(offset, &s_sum, sizeof(s_sum)); //拷贝校验和
		return m_raw_data.data();
	}

	// 获取序列化数据大小
	uint32_t size() {
		data();
		return m_raw_data.size();
	}
};
#pragma pack(pop)

class CServerSocket
{
public:
	static CServerSocket* m_instance;  //仅仅是声明，static必须创建时进行初始化。
	SOCKET m_server_socket;
	SOCKET m_client_socket;
	CPacket m_packet;
	//禁止拷贝和赋值
	CServerSocket(const CServerSocket&) = delete;
	CServerSocket& operator=(const CServerSocket&) = delete;

	static CServerSocket* getInstance() {  //静态函数没有this指针，所以无法访问成员变量
		if (m_instance == NULL) {
			m_instance = new CServerSocket();
			return m_instance;
		}
		else return m_instance;
	}

	static void releaseInstance() {
		if (m_instance != nullptr) {
			delete m_instance;
			m_instance = NULL;
		}
	}

	BOOL initSocket() {
		if (m_server_socket == -1) return false;
		sockaddr_in server_addr;
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = INADDR_ANY;  //s.addr是一个简化的宏。   不要默认服务器只有一个网卡
		server_addr.sin_port = htons(1234);
		if (bind(m_server_socket, (sockaddr*)&server_addr, sizeof(sockaddr)) == -1) return FALSE;
		if(listen(m_server_socket, 1) == -1) return FALSE;
		return true;
	}
	
	bool acceptClient() {
		sockaddr_in client_addr;
		int client_size = sizeof(client_addr);
		m_client_socket = accept(m_server_socket, (sockaddr*)&client_addr, &client_size);
		TRACE("client connected: m_client_socket = %d\n", m_client_socket);
		if (m_client_socket < 0) {
			return false;
		}
		return true;
	}

	int dealCommand() {
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		uint32_t index = 0;  //index表示buffer可写入的位置
		while (true) {
			uint32_t len = recv(m_client_socket, buffer+index, BUFFER_SIZE-index, 0);  //0表示阻塞接收
			if (len <= 0) {
				delete[]buffer;
				return -1;  // 没能构造任何包，继续尝试接收数据
			}
			TRACE("recv %d\r\n", len);
			index += len; // 更新写入位置
			len = index;  // 更新累积字节长度
			//处理命令
			m_packet = CPacket((BYTE*)buffer, len);   // 尝试用累积字节构造包
			if (len > 0) {
				memmove(buffer,buffer+len,BUFFER_SIZE-len); // 解析出包后，清理这个包占用的缓冲区。（只需要将缓冲区剩余字节前移）
				index -= len;  				  // 解决len包含多个包的情况。例如index=100,len=50,意味着后50字节是下一个包的内容，下次解析
				delete[] buffer;
				return m_packet.s_command;
			}
		}
		delete[] buffer;
		return -1;  //其他意外情况
	}

	// 输入原始字节,发送数据
	bool sendByte(const void* p_data, uint32_t size) {
		if (m_server_socket == -1) return false;
		return send(m_client_socket, (const char*)p_data, size, 0) == size;
	}

	// 输入包，发送数据
	bool sendByte(CPacket& packet) {   //这里不加const，因为data会修改对象属性
		if (m_server_socket == -1) return false;
		dump(packet.data(), packet.size());
		return send(m_client_socket, packet.data(), packet.size(), 0) == packet.size();  //这里显然有问题，packet中包含了string，没有展平
	}

	bool getFilePath(std::string &str_path) {
		//2表示获取目录文件，3表示打开文件
		if (m_packet.s_command == 2 || m_packet.s_command == 3 || m_packet.s_command == 4) {
			str_path = m_packet.str_data;
			return true;
		}
		return false; //拒绝其他command非法使用该接口
	}

	bool getMouseEvent(MOUSEEVENT & mouse) {
		if (m_packet.s_command == 5) {
			memcpy(& mouse, m_packet.str_data.data(), sizeof(mouse));
			return true;
		}
		return false;
	}

	CPacket& GetPacket() {
		return m_packet;
	}

	void closeClient() {
		closesocket(m_client_socket);
		m_client_socket = INVALID_SOCKET;
	}

private:

	CServerSocket():
		m_client_socket(INVALID_SOCKET)
	{
		if (initSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境"), _T("初始化错误"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_server_socket = socket(AF_INET, SOCK_STREAM, 0);  //创建套接字
	};

	~CServerSocket() {
		if (m_server_socket > 0) closesocket(m_server_socket);
		WSACleanup();
	}

	BOOL initSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 2), &data) != 0)  //2,2表示winsock2.2版本
		{
			return FALSE;
		}
		return TRUE;
	}


	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;

	// 用于调试输出包数据 (GUI版本)
	void dump(const char* data, size_t size) {
		if (!data || size == 0) return;

		// 1. 输出头部信息
		char headerBuffer[64];
		snprintf(headerBuffer, sizeof(headerBuffer), "[DEBUG] Packet Dump (%zu bytes):\n", size);
		OutputDebugStringA(headerBuffer);

		// 2. 准备行缓冲 (16字节 * 3字符/字节 + 换行符 + 结束符，64字节足够)
		char lineBuffer[128] = { 0 };
		int offset = 0;

		for (size_t i = 0; i < size; ++i) {
			// 格式化单个字节追加到 lineBuffer
			// 注意：必须强转 (unsigned char)，否则 0x80 以上的字节会显示为 FFFFFF80
			int written = snprintf(lineBuffer + offset, sizeof(lineBuffer) - offset,
				"%02X ", (int)(unsigned char)data[i]);

			if (written > 0) offset += written;

			// 每 16 个字节输出一行，或者到了最后一个字节
			if ((i + 1) % 16 == 0 || (i + 1) == size) {
				// 追加换行
				if (offset < sizeof(lineBuffer) - 1) {
					lineBuffer[offset++] = '\n';
					lineBuffer[offset] = '\0';
				}
				// 发送到 VS 输出窗口
				OutputDebugStringA(lineBuffer);

				// 重置缓冲
				offset = 0;
				lineBuffer[0] = '\0';
			}
		}

		OutputDebugStringA("--------------------------------\n");
	}
};

//extern CServerSocket server;