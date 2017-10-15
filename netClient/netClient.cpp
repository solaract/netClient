// netClient.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <time.h>
#pragma comment(lib,"Ws2_32.lib")

#define BUF_SIZE 1024
#define DEFAULT_PORT "27015"
#define FILE_NAME_MAX_SIZE 512

struct FILESEND {
	long id = 0;									//文件包编号
	float size = 0;									//文件大小
	int end = 0;									//文件结束标志
	char name[FILE_NAME_MAX_SIZE];					//文件名
	char content[BUF_SIZE];							//文件包缓冲区
};


int main()
{
	WSADATA wsaData;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	SOCKET ConnectSocket = INVALID_SOCKET;
	SOCKET ClientSocket = INVALID_SOCKET;
	FILESEND fileS, oldFile;
	//char sendbuf[BUF_SIZE];
	char recvbuf[BUF_SIZE];
	//char fileName[File_Name_Max_Size+1];
	int iResult, iSendResult;
	int recvbuflen = BUF_SIZE;
	time_t oldT, nowT;
	//printf("%d", sizeof(fileS));
	//初始化WSA
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult) {
		printf("WSAStartup failed:%d\n", iResult);
		return 1;
	}
	//协议信息
	ZeroMemory(&hints, sizeof(hints));
	//ipv4
	hints.ai_family = AF_INET;
	//TCP
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	//hints.ai_flags = AI_PASSIVE;
	//服务器IP
	iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
	if (iResult) {
		printf("getaddrinfo failed:%d\n", iResult);
		WSACleanup();
		return 1;
	}
	ptr = result;
	do {
		//初始化连接socket
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("Error at socket():%d\n", WSAGetLastError());
			freeaddrinfo(result);
			WSACleanup();
			return 1;
		}
		//连接服务器
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			printf("Unable to connect to server!\n");
			closesocket(ConnectSocket);
			continue;
		}
		//freeaddrinfo(result);
		//ZeroMemory(sendbuf, sizeof(recvbuf));
		ZeroMemory(recvbuf, sizeof(recvbuf));
		ZeroMemory((char*)&fileS, sizeof(fileS));
		//接收欢迎
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("server:%s\n", recvbuf);
			printf("Bytes received: %d\n", iResult);
		}
		//重传跳过文件名
		if (oldFile.id == 0 && oldFile.end == 0) {
			printf("input file name to download:");
			gets_s(oldFile.name);
			printf("\n");
		}
		//发送文件名
		iResult = send(ConnectSocket, oldFile.name, strlen(oldFile.name) + 1, 0);
		if (iResult == SOCKET_ERROR) {
			printf("send failed: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
			continue;
		}
		ZeroMemory(recvbuf, sizeof(recvbuf));
		//接受文件名和大小
		iResult = recv(ConnectSocket, (char*)&fileS, sizeof(fileS), 0);
		if (iResult > 0) {

			//int pos = 0;
			//char size[BUF_SIZE];
			//pos = strpos(recvbuf, ';');

			strcpy_s(oldFile.name, fileS.name);
			oldFile.size = fileS.size;
			//发送文件断点id
			iResult = send(ConnectSocket, (char*)&oldFile, sizeof(oldFile), 0);
			if (iResult == SOCKET_ERROR) {
				printf("send failed: %d\n", WSAGetLastError());
				closesocket(ConnectSocket);
				continue;
			}
			//打开文件
			FILE *fp;
			//重传以追加方式打开
			if (!oldFile.end&&oldFile.id > 0) {
				fopen_s(&fp, oldFile.name, "ab+");
			}else
				oldT = time(NULL);
				fopen_s(&fp, oldFile.name, "wb");
			if (!oldFile.end&&oldFile.id > 0) {
				fseek(fp, BUF_SIZE*oldFile.id, SEEK_CUR);
			}
			if (fp == NULL) {
				printf("file %s can not open", oldFile.name);
				return 1;
			}

			printf("server:file:%s;size:%fKB\n", oldFile.name, oldFile.size);
			ZeroMemory(recvbuf, sizeof(recvbuf));
			ZeroMemory((char*)&fileS, sizeof(fileS));
			//接收文件
			while (iResult = recv(ConnectSocket, (char*)&fileS, sizeof(fileS), 0)) {
				if (iResult > 0) {
					printf("server send file bytes:%d;ID:%ld\n", iResult, oldFile.id);
					//计算文件包ID
					if (oldFile.id != fileS.id) {
						oldFile.id--;
						break;
					}
					else
						oldFile.id++;
					int writeLen = fwrite(fileS.content, sizeof(char), BUF_SIZE, fp);
					
					oldFile.end = fileS.end;
					ZeroMemory((char*)&fileS, sizeof(fileS));
				}
				else {
					printf("recv file %s failed: %d\n", oldFile.name, WSAGetLastError());
					closesocket(ConnectSocket);
					break;
				}
			}
			if (oldFile.end) {
				printf("file %s download finished\n", oldFile.name);
				//计算用时
				nowT = time(NULL);
				printf("Time:%lds\n", nowT - oldT);
			}
			else
				printf("file %s download interrupted,try again\n", oldFile.name);
			fclose(fp);
		}
		else if (iResult == 0)
			printf("Connection closed\n");
		else
			printf("recv failed: %d\n", WSAGetLastError());
		printf("connect close\n");
		closesocket(ConnectSocket);
	}while (!oldFile.end||oldFile.id == 0);
	
	
	//printf("send:");
	//gets_s(sendbuf);
	//printf("\n");
	//while (strlen(sendbuf))
	//{
	//	//ZeroMemory(recvbuf, sizeof(recvbuf));
	//	iResult = send(ConnectSocket, sendbuf, strlen(sendbuf)+1, 0);
	//	if (iResult == SOCKET_ERROR) {
	//		printf("send failed: %d\n", WSAGetLastError());
	//		closesocket(ConnectSocket);
	//		WSACleanup();
	//		return 1;
	//	}
	//	printf("Bytes Sent: %ld\n", iResult);
	//	iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
	//	if (iResult > 0) {
	//		printf("server:%s\n", recvbuf);
	//		printf("Bytes received: %d\n", iResult);
	//	}else if (iResult == 0)
	//		printf("Connection closed\n");
	//	else
	//		printf("recv failed: %d\n", WSAGetLastError());
	//	printf("send:");
	//	gets_s(sendbuf);
	//	printf("\n");
	//}
	
	WSACleanup();
    return 0;
}
