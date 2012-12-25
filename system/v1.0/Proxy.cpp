/*
 * 	This file contains functions of a Proxy.
 *
 * 	By Qichen Pan.
 */

#include "lib.h"
#include "util.h"

int main()
{
	FILE* RM_Info = fopen("RM_Info", "r");
	char RM_IP[80] = "";
	int RM_Port = 0;
	fscanf(RM_Info, "%s", RM_IP);
	fscanf(RM_Info, "%d", &RM_Port);
	fclose(RM_Info);

	//Connect to RM using socket
	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		printf("Error occurs openning socket.\n");
		exit(1);
	}
	struct sockaddr_in RM_Addr;
	struct hostent *RM_Host;
	RM_Addr.sin_family = AF_INET;
	RM_Host = gethostbyname(RM_IP);
		// error check
	memcpy(&RM_Addr.sin_addr.s_addr, RM_Host->h_addr, RM_Host->h_length);
	RM_Addr.sin_port = htons(RM_Port);
	connect(sockfd, (sockaddr*)&RM_Addr, sizeof(RM_Addr));
		// error check
	
	// Initialization
	FILE* Node_Info = fopen("Node_Info", "r");
	char buf[1024] = " ";
	char tmp[20] = "";
	int num = 0;
	long file_len = 0;
	fseek(Node_Info, 0L, SEEK_END);
	file_len = ftell(Node_Info);
	fseek(Node_Info, 0L, SEEK_SET);
	sprintf(buf, "6 ");
	while(ftell(Node_Info) < file_len)
	{
		fscanf(Node_Info, "%d", &num);
		sprintf(tmp, "%d ", num);
		strcat(buf, tmp);
	}
	fclose(Node_Info);
	send(sockfd, buf, 1024, 0);

	// Passing heartbeat message
	sprintf(buf, "7 Ping");
	while(1)
	{
		sleep(10);
		send(sockfd, buf, 1024, 0);
	}
	close(sockfd);
	return 0;
}
