/*
 * This file contains functions of Tasks in our system.
 *
 * by Qichen Pan.
 */

#include "lib.h"
#include "util.h"
#include "/home/pan/mmds/proj/2/user.h"

#define Queue_L 4

void user_program(char*, char*, char*);


int main(int argc, char* argv[])
{
	// get basic information
	char Type[10] = "";
	char Program_Path[512], Input_Path[512];
	int Parameter;
	strcpy(Type, argv[1]);
	sscanf(argv[2], "%d", &Parameter);
	strcpy(Program_Path, argv[3]);
	strcpy(Input_Path, argv[4]);

	// Get TM information
	char TM_IP[80] = "";
	int TM_Port;
	FILE* TM_Info = fopen("TM_Info", "r");
	if(TM_Info == NULL)
	{
		perror("Fail to open file.\n");
		exit(1);
	}
	fscanf(TM_Info, "%s", TM_IP);
	fscanf(TM_Info, "%d", &TM_Port);
	fscanf(TM_Info, "%d", &TM_Port);

	// open socket to connect to TM
	char buf[1024] = "";
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		printf("Error occurs when opening socket.\n");
		exit(1);
	}

	struct sockaddr_in TM_Addr;
	struct hostent* TM_Host;
	TM_Addr.sin_family = AF_INET;
	TM_Host = gethostbyname(TM_IP);
	memcpy(&TM_Addr.sin_addr.s_addr, TM_Host->h_addr, TM_Host->h_length);
	TM_Addr.sin_port = htons(TM_Port);
	while(connect(sockfd, (sockaddr*)&TM_Addr, sizeof(TM_Addr)) == -1)
	{
		sleep(1);
	}
	
	// send device type
	sprintf(buf, "8 %s", Type);
	send(sockfd, buf, 1024, 0);

	// initial Task_Queue
	queue<Split> Task_Queue;
	while(Task_Queue.size() < Queue_L - 1)
	{
		recv(sockfd, buf, 1024, 0);
		if(buf[0] == 'a')
		{
			break;
		}else
		if(buf[0] == 'c')
		{
			int i = 0;
			long lr = 0, os = 0;
			sscanf(buf + sizeof(char) * 2, "%d %ld %ld", &i, &lr, &os);
			Split new_Split(i, lr, os);
			Task_Queue.push(new_Split);
			printf("Get new Split with ID: %d\nNow queue size: %d\n", i, Task_Queue.size());
			sprintf(buf, "d");
			send(sockfd, buf, 1024, 0);
		}else
		{
			printf("Invalid MSG from TM.\n");
			exit(1);
			continue;
		}
	}

	// begin work
	while(1)
	{
		// Test if work is done
		if(Task_Queue.empty())
		{
			printf("Task is done, exit.\n");
			close(sockfd);
			exit(0);
		}
		
		//Finish a split data
		Split Current_Split(Task_Queue.front());
		Task_Queue.pop();
		
		char begin_char[20] = "", offset_char[20] = "";
		sprintf(begin_char, "%ld", Current_Split.begin);
		sprintf(offset_char, "%ld", Current_Split.offset);
		printf("begin: %s, offset: %s\n", begin_char, offset_char);
	/*	int pid = fork();
		int status;
		switch(pid)
		{
			case -1:
				printf("Error fork fails.\n");
				break;
			case 0:
				execl(Program_Path, Program_Path, Input_Path, begin_char, offset_char, (char*)0);
				printf("%s %s %s %s\n", Program_Path, Input_Path, begin_char, offset_char);
				break;
			default:
				wait(&status);
				break;
		}
	*/	
		user_program(Input_Path, begin_char, offset_char);

		Current_Split.Finished_Time = time(NULL);
		printf("Task has finished Split %d at time %ld.\n", Current_Split.Split_Id, Current_Split.Finished_Time);
		// Send this finished Split to TM
		sprintf(buf, "9 %d %ld", Current_Split.Split_Id, Current_Split.Finished_Time);
		send(sockfd, buf, 1024, 0);

		// ask for a new Split
		recv(sockfd, buf, 1024, 0);
		if(buf[0] == 'a')
		{
			//continue;
		}else
		if(buf[0] == 'c')
		{
			int i = 0;
			long lr = 0, os = 0;
			sscanf(buf + sizeof(char) * 2, "%d %ld %ld", &i, &lr, &os);
			Split new_Split(i, lr, os);
			Task_Queue.push(new_Split);
			printf("Get new Split with ID: %d\n", i);
		}else
		{
			printf("Invalid MSG from TM.\n");
			exit(1);
			continue;
		}
	
	//	if(Task_Queue.size() == 0)
	//	{
	//		printf("Task work done.\n");
	//		break;
	//	}
	}
	return 0;
}
