#pragma warning(disable : 4996)
#pragma comment(lib, "wsock32.lib")
#include <WinSock2.h>
#include <stdio.h>
#include <process.h>
#include <string.h>	
#include <conio.h>
#include <ctime>
#include <malloc.h>

void gotoxy(int xpos, int ypos) {
	COORD scrn;
	HANDLE hOuput = GetStdHandle(STD_OUTPUT_HANDLE);
	scrn.X = xpos; scrn.Y = ypos;
	SetConsoleCursorPosition(hOuput, scrn);
}

WSADATA WsaData;
#define MAX_MSG_SIZE	64		//��ƶǿ�W��
#define MaxConnect		20		//�̤j�s�u�ƶq
#define ENTER			0x0D
#define BACKSPACE		0x08

char ServerIP[16];
SOCKET serversocket;
char name_for_ask[100];
char MyName[100];
bool waitIP = true;
u_short ServerPort;			//�ۤv��port
char sendmsg[MAX_MSG_SIZE];

char screen[29][119];

void Server(void*);
void Client(void*);
void waitClient(void* p);
void Lobby();
void Printscreen(void*);
void ReceiveMsg(void*);
void SetScreen();
int Sortmsg(char*);
void RecClientSocket(SOCKET);
int RecMemberName(char*);
void Chatroom(char*name, char* msg);
void Printchat();
void Printtime();
void Printmember();
void PrintCHATROOM();
void PrintOnScreen(const char* words, int locX, int locY);
void Sendmsg(SOCKET aim,char* name, char* msg);

int main() {
	//Enter NAME
	printf("Enter Your Name:");
	scanf("%s",MyName);
	//�]�wport
	printf("Set Your Port:");
	scanf_s("%hu", &ServerPort);
	if (WSAStartup(MAKEWORD(1, 1), &WsaData) != 0) {
		WSACleanup();
		exit(0);
	}
	//�Ψӵ��ݧO�H�s���������
	_beginthread(Server, 0, 0);
	//�Ψӳs���O�H�������
	_beginthread(Client, 0, 0);
	while (waitIP)
		Sleep(100);
	//======>
	while (waitIP)
		Sleep(500);
	
	//=======================================================================================
	Lobby();

}

void Server(void*) {
	while (waitIP)
		Sleep(500);
	SOCKET ServerSocket;
	SOCKADDR_IN ServerAddr;
	//----socket----
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ServerSocket == INVALID_SOCKET) {
		closesocket(ServerSocket);
		WSACleanup();
		exit(0);
	}
	//----set Socket Addr----
	ServerAddr.sin_addr.s_addr = INADDR_ANY;
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(ServerPort);
	//----bind----
	if (bind(ServerSocket, (LPSOCKADDR)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR) {
		closesocket(ServerSocket);
		WSACleanup();
		exit(0);
	}
	//----listen----
	if (listen(ServerSocket, MaxConnect) == SOCKET_ERROR) {
		closesocket(ServerSocket);
		WSACleanup();
		exit(0);
	}
	//----����Client�s��----
	waitClient((void*)ServerSocket);
}

void Client(void*) {
	SOCKET ServerSocket;
	SOCKADDR_IN ServerAddr;
	ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ServerSocket == INVALID_SOCKET) {
		closesocket(ServerSocket);
		WSACleanup();
		exit(0);
	}
	//----�ШD��JIP----
	printf("IP (To be a server enter '0'):");
	scanf_s("%s", ServerIP, 16);
	//�Y��J��"0"�N��@������s��Server
	//�u����Client�ӳs��(be a server)
	if (strcmp(ServerIP, "0") == 0) {
		closesocket(ServerSocket);
		system("CLS");
		waitIP = false;
		return;
	}
	//��J��誺port
	printf("Destination Port:");
	int port;
	scanf_s("%d", &port);
	system("CLS");
	waitIP = false;
	//----set Socket Addr----
	ServerAddr.sin_addr.s_addr = inet_addr(ServerIP);
	ServerAddr.sin_family = AF_INET;
	ServerAddr.sin_port = htons(port);
	//----connect to server----
	if (connect(ServerSocket, (LPSOCKADDR)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR) {
		closesocket(ServerSocket);
		WSACleanup();
		exit(0);
	}
	//�}�l����server�T��
	_beginthread(ReceiveMsg, 0, (void*)ServerSocket);
	//�s�UServer�� SOCKET
	serversocket = ServerSocket;
	//�ǰeClient���W�r��Server
	Sleep(2000);
	sprintf(sendmsg, "name %s", MyName);
	send(ServerSocket, sendmsg, MAX_MSG_SIZE, 0);
}

void waitClient(void* p) {
	SOCKET ServerSocket = (SOCKET)p;
	SOCKET ClientSocket;
	SOCKADDR_IN ClientAddr;
	int ClientAddrLength = sizeof(ClientAddr);
	//����Client�s�u
	ClientSocket = accept(ServerSocket, (struct sockaddr FAR*)&ClientAddr, &ClientAddrLength);
	if (ClientSocket == INVALID_SOCKET) {
		closesocket(ClientSocket);
		WSACleanup();
		return;
	}
	//�s�W����Client�s���������
	_beginthread(waitClient, 0, (void*)ServerSocket);
	//�O����Client��SOCKET
	RecClientSocket(ClientSocket);
	//�}�l������Client�T��
	ReceiveMsg((void*)ClientSocket);
	//�ǰeServer���W�r��Client
	Sleep(3000);
	sprintf(sendmsg, "name %s", MyName);
	send(ClientSocket, sendmsg, MAX_MSG_SIZE, 0);
}

int sp = 0;
SOCKET list[MaxConnect];
void RecClientSocket(SOCKET newclient) {
	list[sp++] = newclient;
}

int  spp = 0;
char *member[MaxConnect];
int RecMemberName(char* name) {
	int length = sizeof(name);

	member[spp] = (char*)malloc(sizeof(char) * 20);
	strcpy(member[spp], name);
	*(member[spp++] + length ) = NULL;
	return (spp - 1);
}

void ReceiveMsg(void* data) {
	char receivemsg[MAX_MSG_SIZE];
	char _name[100];
	char chat_tmp[100];
	int numbering;

	while (true) {
		if (recv((SOCKET)data, receivemsg, MAX_MSG_SIZE, 0) == SOCKET_ERROR) {
			free(member[numbering]);
			member[numbering] = NULL;
			return;
		}
		switch (Sortmsg(receivemsg)) {
		case 1: //line
			sscanf(receivemsg, "line %s %[^\n]", _name, chat_tmp);
			Chatroom(_name, chat_tmp);
			if (sp != 0)                    //Server Announce
				for (int i = 0; i < sp; i++) {
					send(list[i], receivemsg, MAX_MSG_SIZE, 0);
				}
			break;
		case 2: //name
			sscanf(receivemsg, "name %s", _name);
			numbering = RecMemberName(_name);
		default:
			break;
		}
	}
}

int index = 0;
char typebuf[100];
char typespace[100];

void Lobby() {
	_beginthread(Printscreen, 0, 0);

	int c;
	while (true) {
		switch (c = _getch()) {
		case BACKSPACE:
			if (index > 0) {
				index--;
				typebuf[index] = '\0';
			}
			break;
		case ENTER:
			if (index != 0) {
				if (sp == 0) {//i am client
					Sendmsg(serversocket, MyName, typebuf);
				}
				else {
					for (int i = 0; i < sp; i++)
						Sendmsg(list[i], MyName, typebuf);
				}
				if (sp != 0)              //server�ۤv���
					Chatroom(MyName, typebuf);
				index = 0;
				typebuf[index] = 0;
			}
			break;
		default:
			if (index < 99) {
				typebuf[index++] = c;
				typebuf[index] = '\0';
			}
			break;
		}
	}
}

void Printscreen(void*) {
	while (true) {
		Sleep(20);
		SetScreen();
		Printchat();
		Printtime();
		Printmember();
		PrintCHATROOM();
		gotoxy(0, 0);
		printf((char*)screen);
//printf typebuf
		gotoxy(1,20);
		printf("%s", typebuf);
	}
}

int Sortmsg(char* msg) {
	char check[20];
	sscanf(msg, "%s ", check);
	if (strcmp(check, "line") == 0)
		return 1;
	if (strcmp(check, "name") == 0)
		return 2;
	return 0;
}

void Sendmsg(SOCKET aim,char* name,char* msg) {
	sprintf(sendmsg, "line %s %s",name, msg);
	send(aim, sendmsg, MAX_MSG_SIZE, 0);
}

char *chatrec[18];

void Chatroom(char*name, char* msg) {
	//  18��
	static int i = 0;

	int len1 = 0,len2=0;
	while (msg[len1++]);
	while (name[len2++]);
	char *tmp = (char*)malloc(sizeof(char) * (len1 + len2 + 6));
	sprintf(tmp, "[%s] : %s", name, msg);

	if (i < 18)
		chatrec[i++] = tmp;
	else {
		free(chatrec[0]);
		for (int i = 0; i < 17; i++) {
			chatrec[i] = chatrec[i + 1];
		}
		chatrec[17] = tmp;
	}
}

void Printchat() {
	for (int i = 0; i < 18; i++) {
		if (!chatrec[i] == NULL)
			PrintOnScreen(chatrec[i], 1, 1 + i);
	}
}

void Printtime() {
	time_t now = time(0);
	char* dt = ctime(&now);

	char week[5];
	char month[5];
	char date[5];
	char time[10];
	sscanf(dt, "%s %s %s %s", week, month, date, time);
	char *tmp1 = NULL;
	tmp1 = (char*)malloc(sizeof(char) * 11);
	sprintf(tmp1, "%s %s %s", month, date, week);
	PrintOnScreen(tmp1, 103, 23);
	//==
	char *tmp2 = NULL;
	tmp2 = (char*)malloc(sizeof(char) * 11);
	sprintf(tmp2, "%s", time);
	PrintOnScreen(tmp2, 104, 26);
}

void Printmember() {
	//gotoxy(104, 3);

	for (int i = 0; i < MaxConnect; i++) {
		if (member[i] != NULL) {
			int length = 0;  //length of words + 1(NULL)
			while (member[length++]);
			//==========
			char *tmp = NULL;
			tmp = (char*)malloc(sizeof(char) * (length + 2));
			sprintf(tmp, "[%s]", member[i]);
			PrintOnScreen(tmp, 104, 3 + i);
		}
	}
}

void PrintOnScreen(const char* words, int locX, int locY) {
	int length = 0;  //length of words + 1(NULL)
	while (words[length++]);
	length--;        //length of words

	for (int i = 0; i < length; i++) {
		screen[locY][locX + i] = *(words + i);
	}
}

const char *row1 = "   ####    #      #       ##       ########    ######      #####      #####        #     #       "; //98
const char *row2 = "  #        #      #      #  #          #       #     #    #     #    #     #      # #   # #      ";
const char *row3 = " #         #      #     #    #         #       #     #    #     #    #     #     #   # #   #     ";
const char *row4 = " #         ########    ########        #       ######     #     #    #     #     #    #    #     ";
const char *row5 = "  #        #      #    #      #        #       #    #     #     #    #     #    #           #    ";
const char *row6 = "   ####    #      #    #      #        #       #     #     #####      #####     #           #    ";

void PrintCHATROOM() {

	PrintOnScreen(row1, 1, 22);
	PrintOnScreen(row2, 1, 23);
	PrintOnScreen(row3, 1, 24);
	PrintOnScreen(row4, 1, 25);
	PrintOnScreen(row5, 1, 26);
	PrintOnScreen(row6, 1, 27);
	
}

void SetScreen() {
	//======================================screen===========================================
	for (int i = 0; i < 28; i++)
		screen[i][118] = '\n';
	screen[28][118] = '\0';
	//===========
	for (int i = 0; i < 29; i++)
		for (int j = 0; j < 118; j++)
			screen[i][j] = ' ';
	//==
	for (int i = 0; i < 118; i++)
		screen[0][i] = '*';
	for (int i = 0; i < 118; i++)
		screen[28][i] = '*';
	for (int i = 0; i < 118; i++)
		screen[21][i] = '*';
	for (int i = 0; i < 98; i++)
		screen[19][i] = '*';
	//==
	for (int i = 0; i < 28; i++)
		screen[i][0] = '*';
	for (int i = 0; i < 28; i++)
		screen[i][117] = '*';
	for (int i = 0; i < 28; i++)
		screen[i][98] = '*';
	//print Online:
	if (strcmp(ServerIP, "0") == 0) {
		screen[1][105] = 'O';
		screen[1][106] = 'n';
		screen[1][107] = 'l';
		screen[1][108] = 'i';
		screen[1][109] = 'n';
		screen[1][110] = 'e';
		screen[1][111] = ':';
	}
}

