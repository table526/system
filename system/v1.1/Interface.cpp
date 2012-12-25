/*
 * This file contains basic functions of Interface.
 *
 * by Qichen Pan.
 */

#include "util.h"
#include "lib.h"

int CM_Port = 0, TM_Port = 0;
char CM_IP[80] = "", TM_IP[80] = "";
char Source_Code_Path[512] = "", Program_Path[512] = "", Input_Path[512] = "";
int Device_Parameter = -1;

void* conn_to_TM(void*);
void* conn_to_CM(void*);

int main()
{
	// Get CM information 
	FILE* CM_Info;
	CM_Info = fopen("CM_Info", "r");
	fscanf(CM_Info, "%s", CM_IP);
	fscanf(CM_Info, "%d", &CM_Port);
	fclose(CM_Info);

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
	struct sockaddr_in CM_Addr;
	struct hostent *CM_Host;
	CM_Addr.sin_family = AF_INET;
	CM_Host = gethostbyname(CM_IP);

	memcpy(&CM_Addr.sin_addr.s_addr, CM_Host->h_addr, CM_Host->h_length);
	CM_Addr.sin_port = htons(CM_Port);
	while(connect(sockfd_CM, (sockaddr*)&CM_Addr, sizeof(CM_Addr)) == -1)
	{
		sleep(1);
	}

	while(strcmp(Source_Code_Path, "") == 0)
	{
		sleep(1);
	}
	char tmp[1024] = "";
	sprintf(tmp, "0 %s", Source_Code_Path);
	send(sockfd_CM, tmp, 1024, 0);
	recv(sockfd_CM, tmp, 1024, 0);

	while(1)
	{
		if(tmp[0] == '1')
			break;
		else
		{
			sleep(1);
			recv(sockfd_CM, tmp, 1024, 0);
		}
	}
	sscanf(tmp + 2 * sizeof(char), "%s", Program_Path);
	printf("Program_Path: %s\n", Program_Path);

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
	while(connect(sockfd_TM, (sockaddr*)&TM_Addr, sizeof(TM_Addr)) == -1)
	{
		sleep(1);
	}
	while(strcmp(Program_Path, "") == 0)
	{
		sleep(1);
	}
	sprintf(tmp, "2 %s %d %s", Program_Path, Device_Parameter, Input_Path);
	
	send(sockfd_TM, tmp, 1024, 0);
	recv(sockfd_TM, tmp, 1024, 0);
	while(tmp[0] != 'b')
	{
		sleep(1);
		recv(sockfd_TM, tmp, 1024, 0);
	}
	printf("Job is done!.\nExit.\n");
	close(sockfd_TM);
	return NULL;
}
