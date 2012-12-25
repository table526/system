/*
 * This file contains functions of Compiler in our system.
 *
 * by Qichen Pan.
 */

#include "util.h"
#include "lib.h"

int CM_Port = 0;
char CM_IP[80] = "";

int main()
{
	// Get information of 
	FILE* CM_Info = fopen("CM_Info", "r");
	fscanf(CM_Info, "%s", CM_IP);
	fscanf(CM_Info, "%d", &CM_Port);
	fclose(CM_Info);

	// open socket connecting to IF
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("Error occurs when opening socket.\n");
		exit(1);
	}
	struct sockaddr_in CM_Addr;
	CM_Addr.sin_family = AF_INET;
	CM_Addr.sin_addr.s_addr = htonl(INADDR_ANY);
	CM_Addr.sin_port = htons(CM_Port);
	bind(sockfd, (struct sockaddr*)& CM_Addr, sizeof(CM_Addr));
	while(1)
	{
		listen(sockfd, 5);

		int IF_Client;
		struct sockaddr_in IF_Addr;
		socklen_t IF_Sock_Len = sizeof(sockaddr_in);
		IF_Client = accept(sockfd, (struct sockaddr*)&IF_Addr, &IF_Sock_Len);

		char Source_Code_Path[512] = "", Program_Path[512] = "";
		char tmp[1024] = "";
	
	
		recv(IF_Client, tmp, 1024, 0);
		while(tmp[0] != '0')
		{
			recv(IF_Client, tmp, 1024, 0);
		}
		sscanf(tmp + sizeof(char) * 2, "%s", Source_Code_Path);
		printf("Source_Code_Path: %s\n", Source_Code_Path);

		/* Compile Method */
		strcpy(Program_Path, Source_Code_Path);
	
		printf("Program_Path: %s\n", Program_Path);
		sprintf(tmp, "1 %s", Program_Path);
		send(IF_Client, tmp, 1024, 0);
	}
	close(sockfd);
	return 0;
}
