/*
 *	This file contains basic functions of RM in our system.
 *
 *	By Qichen Pan.
 */

#include "util.h"
#include "lib.h"

int main()
{
	// Open socket connecting to Proxy
	FILE* RM_Info = fopen("RM_Info", "r");
	int RM_Port = 0;
	char RM_IP[80] = "";
	fscanf(RM_Info, "%s", RM_IP);
	fscanf(RM_Info, "%d", &RM_Port);
	fclose(RM_Info);

	int sockfd;
	const int max_client_num = 80;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		perror("Error occurs opening socket.\n");
		exit(1);
	}

	struct sockaddr_in RM_addr;
	RM_addr.sin_family = AF_INET;
	RM_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	RM_addr.sin_port = htons(RM_Port);
	bind(sockfd, (struct sockaddr*)& RM_addr, sizeof(RM_addr));

	listen(sockfd, max_client_num);

	int Proxy_sockfd;
	struct sockaddr_in Proxy_addr;
	socklen_t sock_len = sizeof(sockaddr_in);
	Proxy_sockfd = accept(sockfd, (struct sockaddr*)&Proxy_addr, &sock_len);

	// Deal with initial infomation
	char buf[1024];
	recv(Proxy_sockfd, buf, 1024, 0);
	if(buf[0] == '6')
	{
		printf("%s\n", buf);
	}

	// Deal with heartbeat infomation
	recv(Proxy_sockfd, buf, 1024, 0);
	if(buf[0] == '7')
	{
		printf("%s\n", buf);
	}

	close(Proxy_sockfd);
	return 0;
}
