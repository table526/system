/*
 * This file contains functions of Compiler in our system.
 *
 * by Qichen Pan.
 */

#include "util.h"
#include "lib.h"

int IF_Port = 0;
char IF_IP[80] = "";

int main()
{
	// Get information of 
	FILE* IF_Info = fopen("IF_Info", "r");
	fscanf(IF_Info, "%s", IF_IP);
	fscanf(IF_Info, "%d", &IF_Port);
	fclose(IF_Info);

	// open socket connecting to IF
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("Error occurs when opening socket.\n");
		exit(1);
	}
	struct sockaddr_in IF_Addr;
	struct hostent *IF_Host;
	IF_Addr.sin_family = AF_INET;
	IF_Host = gethostbyname(IF_IP);
		//error check
	memcpy(&IF_Addr.sin_addr.s_addr, IF_Host->h_addr, IF_Host->h_length);
	IF_Addr.sin_port = htons(IF_Port);
	bzero(&(IF_Addr.sin_zero), 8);
	while(connect(sockfd, (sockaddr*)&IF_Addr, sizeof(IF_Addr)) == -1);

	char Source_Code_Path[512] = "", Program_Path[512] = "";
	char tmp[1024] = "";
	
	recv(sockfd, tmp, 1024, 0);
	while(tmp[0] != '0')
	{
		recv(sockfd, tmp, 1024, 0);
	}
	sscanf(tmp + sizeof(char) * 2, "%s", Source_Code_Path);
	printf("Source_Code_Path: %s\n", Source_Code_Path);

	/* Compile Method */
	strcpy(Program_Path, Source_Code_Path);
	
	printf("Program_Path: %s\n", Program_Path);
	sprintf(tmp, "1 %s", Program_Path);
	send(sockfd, tmp, 1024, 0);

	close(sockfd);
	return 0;
}
