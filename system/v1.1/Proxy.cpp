/*
 * 	This file contains functions of a Proxy.
 *
 * 	By Qichen Pan.
 */

#include "lib.h"
#include "util.h"

void* send_heart_beat(void*);
void* fork_task(void*);

char buf[1024] = "";
char Program_Path[512] = "", Input_Path[512] = "";
int sockfd;
Node local_Node;

int main()
{
	FILE* RM_Info = fopen("RM_Info", "r");
	char RM_IP[80] = "";
	int RM_Port = 0;
	fscanf(RM_Info, "%s", RM_IP);
	fscanf(RM_Info, "%d", &RM_Port);
	fclose(RM_Info);

	//Connect to RM using socket
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
	char tmp[20] = "";
	int num = 0;
	long file_len = 0;
	fseek(Node_Info, 0L, SEEK_END);
	file_len = ftell(Node_Info);
	fseek(Node_Info, 0L, SEEK_SET);
	sprintf(buf, "6 ");
	while(ftell(Node_Info) < file_len - 1)
	{
		fscanf(Node_Info, "%d", &num);
		sprintf(tmp, "%d ", num);
		strcat(buf, tmp);
	}
	fseek(Node_Info, 0L, SEEK_SET);
	fscanf(Node_Info, "%d %d", &local_Node.CPU_num, &local_Node.GPU_num);
	local_Node.CPU_Core_num = (int*)malloc(sizeof(int) * local_Node.CPU_num);
	local_Node.GPU_Model = (int*)malloc(sizeof(int) * local_Node.GPU_num);
	for(int i = 0; i < local_Node.CPU_num; i++)
	{
		fscanf(Node_Info, "%d", &local_Node.CPU_Core_num[i]);
	}
	for(int i = 0; i < local_Node.GPU_num; i++)
	{
		fscanf(Node_Info, "%d", &local_Node.GPU_Model[i]);
	}
	fclose(Node_Info);
	send(sockfd, buf, 1024, 0);

	// Creating thread to pass heartbeat message
	pthread_t thread_send_heart_beat, thread_fork_task;
	pthread_create(&thread_send_heart_beat, NULL, send_heart_beat, NULL);

	// Creating thread to fork task
	pthread_create(&thread_fork_task, NULL, fork_task, NULL);

	pthread_join(thread_send_heart_beat, NULL);
	free(local_Node.CPU_Core_num);
	free(local_Node.GPU_Model);
	return 0;
}

void* send_heart_beat(void* args)
{
	sprintf(buf, "7 Ping");
	while(1)
	{
		sleep(10);
		printf("Beat.\n");
		send(sockfd, buf, 1024, 0);
	}
	return NULL;
}

void* fork_task(void* args)
{
	int status;
	char buf_code[1024] = "";
	while(1)
	{
		recv(sockfd, buf_code, 1024, 0);
		if(buf_code[0] != '5')
		{
			perror("Invalid MSG from RM.\n");
			exit(1);
		}
		sscanf(buf_code + sizeof(char) * 2, "%s %s", Program_Path, Input_Path);
		printf("Program_Path: %s\nInput Path: %s\n", Program_Path, Input_Path);
		// fork Task
		int cpu_pid[local_Node.CPU_num];
		int gpu_pid[local_Node.GPU_num];
		char tmp_cpu[20] = "";
		char tmp_gpu[20] = "";
		for(int c = 0; c < local_Node.CPU_num; c++)
		{
			cpu_pid[c] = fork();
			switch(cpu_pid[c])
			{
				case(-1): 
					printf("Error: fail to fork process.\n");
					break;
				case(0):
					sprintf(tmp_cpu, "%d", local_Node.CPU_Core_num[c]);
					execl("./TASK", "TASK", "CPU", tmp_cpu, Program_Path, Input_Path, (char*)0);
					break;
				default:
					printf("A new CPU Task started.\n");
					break;
			}
		}
		for(int g = 0; g < local_Node.GPU_num; g++)
		{
			gpu_pid[g] = fork();
			switch(gpu_pid[g])
			{
				case(-1): 
					printf("Error: fail to fork process.\n");
					break;
				case(0):
					sprintf(tmp_gpu, "%d", local_Node.GPU_Model[g]);
					execl("./TASK", "TASK", "GPU", tmp_gpu, Program_Path, Input_Path,(char*)0);
					break;
				default:
					printf("A new GPU Task started.\n");
					break;
			}
		}
		wait(&status);
		//printf("Task All complete!\n");

	}
	return NULL;
}

