#include "stdafx.h"
#include "define.h"
#include "ServerItem.h"
#include <vector>
#include <time.h>



int logMessage(char* message)
{
	FILE* logFile;
	fopen_s(&logFile,"ServerLog.txt", "a");

	char wday[7][4] = { "Sun","Mon","Tue","Wed","Thu","Fri","Sat" };
	time_t timep;
	struct tm p;
	time(&timep);
	localtime_s(&p, &timep);
	fprintf(logFile, "%d/%d/%d ", (1900 + p.tm_year), (1 + p.tm_mon), p.tm_mday);
	fprintf(logFile, "%s %d:%d:%d\t", wday[p.tm_wday], p.tm_hour, p.tm_min, p.tm_sec);
	fprintf(logFile, "%s\n", message);
	fclose(logFile);
	return 1;
}


int main(int argc, _TCHAR* argv1[])
{
	vector<ServerItem*>servers;

	int port = 69;

	cout << "Please set the listening port: ";
	cin >> port;


	WORD myVersionRequest;
	WSADATA wsaData;
	myVersionRequest = MAKEWORD(1, 1);
	int ret;
	ret = WSAStartup(myVersionRequest, &wsaData);
	if (ret != 0) {
		cout << "Socket Opened Error!" << endl;
		return 0;
	}


	SOCKET serverSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (serverSocket == INVALID_SOCKET) {
		cout << "Socket() failed!" << endl;
		WSACleanup();
		return 0;
	}

	int imode = 1;
	ret = ioctlsocket(serverSocket, FIONBIO, (u_long*)&imode);
	if (ret == SOCKET_ERROR) {
		cout << "ioctlsocket() failed!" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}


	SOCKADDR_IN addr;
	addr.sin_addr.S_un.S_addr = INADDR_ANY;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	ret = bind(serverSocket, (SOCKADDR*)&addr, sizeof(addr));
	if (ret == SOCKET_ERROR) {
		cout << "Bind() failed!" << endl;
		closesocket(serverSocket);
		WSACleanup();
		return 0;
	}

	cout << "This is my socket server!" << endl;

	char* data = (char*)malloc(MAX_BUFFER_LENGTH);

	struct fd_set fds;
	//struct timeval timeout = { 0, 0 };
	while (1) {
		FD_ZERO(&fds);
		FD_SET(serverSocket, &fds);
		//switch (select(serverSocket + 1, &fds, &fds, NULL, &timeout)) {
		switch (select(serverSocket + 1, &fds, NULL, NULL, NULL)) {
		case -1:
			cout << "Select() Error!" << endl;
			return 0;
			break;
		case 0:
			break;
		default:
			if (FD_ISSET(serverSocket, &fds)) {
				SOCKADDR_IN cAddr;
				int len = sizeof(cAddr);
				ret = recvfrom(serverSocket, data, MAX_BUFFER_LENGTH, 0, (SOCKADDR*)&cAddr, &len);

				if (ret > 0) {
					string x = string();
					x.append(inet_ntoa(cAddr.sin_addr));
					x.append(" Message received: \t");
					char aa[5];
					_itoa_s(ret, aa, 10);
					x.append(aa);
					logMessage((char*)x.c_str());

					vector<ServerItem*>::iterator it;
					for (it = servers.begin(); it != servers.end(); it++) {
						if (((*it)->addr.sin_addr.S_un.S_addr == cAddr.sin_addr.S_un.S_addr) && (*it)->addr.sin_port == cAddr.sin_port) {
							break;
						}
					}
					ServerItem* cserver;
					if (it != servers.end()) {
						cserver = *it;
						package* pac = (package*)data;
						unsigned short opCode = ntohs(pac->opCode);
						if (opCode == TFTP_OP_READ || opCode == TFTP_OP_WRITE) {
							servers.erase(it);
							delete(cserver);

							cout << "[RENew] " << inet_ntoa(cAddr.sin_addr) << " connected." << endl;
							cserver = new ServerItem(&serverSocket, cAddr, data, ret);
							if (cserver->fileError) {
								delete(cserver);
							}
							else {
								servers.push_back(cserver);
							}
						}
						else if (opCode == TFTP_OP_ACK) {

							unsigned short index = ntohs(pac->code);
							cout << inet_ntoa(cAddr.sin_addr) << ": ACK " << index << endl;
							int ret = cserver->handleAck(index);
							if (ret == 1) {
								cout << inet_ntoa(cAddr.sin_addr) << ": " << cserver->filename << "sent successfully!" << endl;
								servers.erase(it);
								delete(cserver);
							}
						}
						else if (opCode == TFTP_OP_DATA) {
							unsigned short index = ntohs(pac->code);
							cout << inet_ntoa(cAddr.sin_addr) << ": DATA " << index << endl;
							int out = cserver->handleData(data, ret);
							if (out == 1) {
								cout << inet_ntoa(cAddr.sin_addr) << ": " << cserver->filename << "save successfully!" << endl;
								servers.erase(it);
								delete(cserver);
							}
						}
						else {
							cout << inet_ntoa(cAddr.sin_addr) << ": Error " << TFTP_ERR_UNEXPECTED_OPCODE << endl;
							cserver->sendErr(TFTP_ERR_UNEXPECTED_OPCODE);
						}
					}
					else {
						package* pac = (package*)data;
						if (ntohs(pac->opCode) == TFTP_OP_READ || ntohs(pac->opCode) == TFTP_OP_WRITE) {
							cout << "[New] " << inet_ntoa(cAddr.sin_addr) << ": Connected." << endl;
							cserver = new ServerItem(&serverSocket, cAddr, data, ret);
							if (cserver->fileError) {
								delete(cserver);
							}
							else {
								servers.push_back(cserver);
							}

						}
					}
				}
				memset(data, 0, MAX_BUFFER_LENGTH);
				ret = 0;
			}
			else {

			}
		}

	}

	getchar();
	return 0;
}
