/*
 * This file contains basic functions of Interface.
 *
 * by Qichen Pan.
 */

#include "util.h"
#include "lib.h"

int IF_Port = 0, TM_Port = 0;
char IF_IP[80] = "", TM_IP[80] = "";
char Source_Code_Path[512] = "", Program_Path[512] = "", Input_Path[512] = "";
int Device_Parameter = -1;

void* conn_to_TM(void*);
void* conn_to_CM(void*);

int main()
{
	// Get IF information 
	FILE* IF_Info;
	IF_Info = fopen("IF_Info", "r");
	fscanf(IF_Info, "%s", IF_IP);
	fscanf(IF_Info, "%d", &IF_Port);
	fclose(IF_Info);

	// Get TM information
	FILE* TM_Info = fopen("TM_Info", "r");
	fscanf(TM_Info, "%s", TM_IP);
	fscanf(TM_Info, "%d", &TM_Port);
	fscanf(TM_Info, "%d", &TM_Port);
	fscanf(TM_Info, "%d", &TM_Port);
	
	// Get user information
	printf("Please type in the source code path: \n");
	scanf("%s", Source_Code_Path);
	printf("Please typpe in the input file path: \n");
	scanf("%s", Input_Path);
	printf("Please type in the device parameter: \n");
	scanf("%d", &Device_Parameter);
	
	// Create thread to deal with CM
	pthread_t thread_conn_to_CM;
	pthread_create(&thread_conn_to_CM, NULL, conn_to_CM, NULL);

	// Create thread to deal with TM
	pthread_t thread_conn_to_TM;
	pthread_create(&thread_conn_to_TM, NULL, conn_to_TM, NULL);

	pthread_join(thread_conn_to_CM, NULL);
	pthread_join(thread_conn_to_TM, NULL);
	return 0;
}

void* conn_to_CM(void* args)
{
	int sockfd_CM = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd_CM < 0)
	{
		perror("Error opening socket.\n");
		exit(1);
	}
	struct sockaddr_in IF_CM_Addr;
	IF_CM_Addr.sin_family = AF_INET;
	IF_CM_Addr.sin_addr.s_addr = htonl(INADDR_ANY);
	IF_CM_Addr.sin_port = htons(IF_Port);
	bind(sockfd_CM, (struct sockaddr*)& IF_CM_Addr, sizeof(IF_CM_Addr));
	listen(sockfd_CM, 5);

	int CM_Client;
	struct sockaddr_in CM_Addr;
	socklen_t CM_Sock_Len = sizeof(sockaddr_in);
	CM_Client = accept(sockfd_CM, (struct sockaddr*)&CM_Addr, &CM_Sock_Len);

	while(strcmp(Source_Code_Path, "") == 0);
	char tmp[1024] = "";
	sprintf(tmp, "0 %s", Source_Code_Path);
	send(CM_Client, tmp, 1024, 0);
	recv(CM_Client, tmp, 1024, 0);

	while(1)
	{
		if(tmp[0] == '1')
			break;
		else
			recv(CM_Client, tmp, 1024, 0);
	}
	sscanf(tmp + 2 * sizeof(char), "%s", Program_Path);
	printf("Program_Path: %s\n", Program_Path);

	close(CM_Client);
	close(sockfd_CM);
	return NULL;
}

void* conn_to_TM(void* args)
{
	char tmp[1024] = "";
	
	int sockfd_TM = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd_TM < 0)
	{
		printf("Error occurs when opening socket.\n");
		exit(1);
	}
	struct sockaddr_in TM_Addr;
	struct hostent *TM_Host;
	TM_Addr.sin_family = AF_INET;
	TM_Host = gethostbyname(TM_IP);
		//error check
	memcpy(&TM_Addr.sin_addr.s_addr, TM_Host->h_addr, TM_Host->h_length);
	TM_Addr.sin_port = htons(TM_Port);
	while(connect(sockfd_TM, (sockaddr*)&TM_Addr, sizeof(TM_Addr)) == -1);
	
	while(strcmp(Program_Path, "") == 0);

	sprintf(tmp, "2 %s %d %s", Program_Path, Device_Parameter, Input_Path);
	send(sockfd_TM, tmp, 1024, 0);
	
	recv(sockfd_TM, tmp, 1024, 0);
	while(tmp[0] != 'b')
	{
		recv(sockfd_TM, tmp, 1024, 0);
	}
	return NULL;
}
