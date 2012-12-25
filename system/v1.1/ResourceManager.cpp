/*
 *	This file contains basic functions of RM in our system.
 *
 *	By Qichen Pan.
 */

#include "util.h"
#include "lib.h"

void* conn_to_Proxy(void*);
void* conn_to_TM(void*);

int RM_Port = 0, TM_Port = 0;
char RM_IP[80] = "", TM_IP[80] = "";
char Program_Path[512] = "", Input_Path[512];
int Device_Parameter = -1;
int CPU_Task_num = 0, GPU_Task_num = 0;
int passing_time = 0;
map<int, Node> Node_Pool;
map<int, Node>::iterator iter;

int main()
{
	// Get RM information
	FILE* RM_Info = fopen("RM_Info", "r");
	fscanf(RM_Info, "%s", RM_IP);
	fscanf(RM_Info, "%d", &RM_Port);
	fclose(RM_Info);
	
	// Get TM information
	FILE* TM_Info = fopen("TM_Info", "r");
	fscanf(TM_Info, "%s", TM_IP);
	fscanf(TM_Info, "%d", &TM_Port);
	fclose(TM_Info);

	// Creating thread to deal with Proxy
	pthread_t thread_conn_to_Proxy;
	pthread_create(&thread_conn_to_Proxy, NULL, conn_to_Proxy, NULL);

	// Creating thread to deal with TM
	pthread_t thread_conn_to_TM;
	pthread_create(&thread_conn_to_TM, NULL, conn_to_TM, NULL);
	
	pthread_join(thread_conn_to_Proxy, NULL);
	pthread_join(thread_conn_to_TM, NULL);

	for(iter = Node_Pool.begin(); iter != Node_Pool.end(); iter++)
	{
		if(iter->second.CPU_Core_num != NULL)
			free(iter->second.CPU_Core_num);
		if(iter->second.GPU_Model != NULL)
			free(iter->second.GPU_Model);
	}
	return 0;
}

void* conn_to_Proxy(void* args)
{
	const int MAX_PROXY_NUM = 256;
	int listenfd, connfd, tmpfd, epfd, nfds, i;
	char buf_P[1024];
	socklen_t Proxy_len;

	struct epoll_event ev, events[MAX_PROXY_NUM];
	epfd = epoll_create(MAX_PROXY_NUM);
	struct sockaddr_in RM_addr, Proxy_addr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	ev.data.fd = listenfd;
	ev.events = EPOLLIN ;//| EPOLLET;

	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
	bzero(&RM_addr, sizeof(RM_addr));
	RM_addr.sin_family = AF_INET;
	RM_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	RM_addr.sin_port = htons(RM_Port);

	bind(listenfd, (sockaddr*)&RM_addr, sizeof(RM_addr));
	listen(listenfd, MAX_PROXY_NUM);

	while(1)
	{
		nfds = epoll_wait(epfd, events, 256, 500);

		for(i = 0; i < nfds; i++)
		{
			if(events[i].data.fd == listenfd)// A new Proxy connecting to RM
			{
				connfd = accept(listenfd, (sockaddr*)&Proxy_addr, &Proxy_len);
				if(connfd < 0)
				{
					perror("Error occurs when Proxy wants to connect to RM.\n");
					exit(1);
				}


				ev.data.fd = connfd;
				ev.events = EPOLLIN ;//| EPOLLET;

				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
			}else if(events[i].events & EPOLLIN)// Proxy wants to send something 
			{
				tmpfd = events[i].data.fd;
				if(tmpfd < 0) 
				{
					epoll_ctl(epfd, EPOLL_CTL_DEL, tmpfd, &ev);
					continue;
				}
				if(recv(tmpfd, buf_P, 1024, 0) < 0)
				{
					printf("Error occurs when fetching from Proxy with ID: %d.\n", events[i].data.fd);
					strcpy(buf_P, "");
					epoll_ctl(epfd, EPOLL_CTL_DEL, tmpfd, &ev);
					continue;
				}

				if(buf_P[0] == '6')
				{
					// deal with initial MSG
					printf("A new Proxy connected. Id: %d\n", events[i].data.fd);
					int c = 0, g = 0, k = 0;
					sscanf(buf_P + sizeof(char) * 2, "%d %d", &c, &g);
					Node *new_Node = new Node(c, g);
					printf("CPU_num: %d\nGPU_num: %d\nCPU_Core_num: ", c, g);
					for(k = 0; k < new_Node->CPU_num; k++)
					{
						sscanf(buf_P + sizeof(char) * (6 + 2 * k), "%d", &new_Node->CPU_Core_num[k]);
						printf("%d ", new_Node->CPU_Core_num[k]);
					}
					printf("\nGPU_Model: ");
					for(k = 0; k < new_Node->GPU_num; k++)
					{
						sscanf(buf_P + sizeof(char) * (6 + 2 * new_Node->CPU_num + 2 * k), "%d", &new_Node->GPU_Model[k]);
						printf("%d ", new_Node->GPU_Model[k]);
					}
					new_Node->alive = time(NULL);
					printf("\nConnection time: %ld\n", new_Node->alive);
					
					Node_Pool.insert(pair<int, Node>(events[i].data.fd, *new_Node));
					strcpy(buf_P, "");
				}else if(buf_P[0] == '7')
				{
					// deal with heart beat MSG
					iter = Node_Pool.find(events[i].data.fd);
					printf("Id:%d\tLast heartbeat:%ld\tNew heartbeat:%ld\n", events[i].data.fd, iter->second.alive, time(NULL));
					iter->second.alive = time(NULL);
					strcpy(buf_P, "");
				}else
				{
				//	printf("Invalid MSG from Proxy with ID: %d.\n", events[i].data.fd);
					strcpy(buf_P, "");
				}

				ev.data.fd = tmpfd;
				ev.events = EPOLLOUT ;//| EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_MOD, tmpfd, &ev);
			}else if(events[i].events & EPOLLOUT)// Proxy wants to fetch something
			{
				// Passing program address to Proxy
				tmpfd = events[i].data.fd;
				if(strcmp(Program_Path, "") != 0)
				{
					if(passing_time < Node_Pool.size())
					{
						char tmp_buf[1024] = "";
						sprintf(tmp_buf, "5 %s %s", Program_Path, Input_Path);
						send(tmpfd, tmp_buf,  1024, 0);
						passing_time++;
					}
					else
					{
						printf("Refresh.\n");
						passing_time = 0;
						strcpy(Program_Path, "");
						strcpy(Input_Path, "");
					}
				}
				ev.data.fd = tmpfd;
				ev.events = EPOLLIN ;//| EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_MOD, tmpfd, &ev);
			}
			else
			{
				// do nothing
			}
			for(iter = Node_Pool.begin(); iter != Node_Pool.end(); iter++)
			{
				if((time(NULL) - iter->second.alive) > 20)
				{
					printf("Id:%d\tis not acting for more than 20s. Delete.\n", iter->first);
					epoll_ctl(epfd, EPOLL_CTL_DEL, iter->first, &ev);
					Node_Pool.erase(iter);
				}
			}
		}
	}
}

void* conn_to_TM(void* args)
{
	int sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0)
	{
		printf("Error occurs opening socket.\n");
		exit(1);
	}
	struct sockaddr_in TM_Addr;
	struct hostent *TM_Host;
	TM_Addr.sin_family = AF_INET;
	TM_Host = gethostbyname(TM_IP);
		//error check
	memcpy(&TM_Addr.sin_addr.s_addr, TM_Host->h_addr, TM_Host->h_length);
	TM_Addr.sin_port = htons(TM_Port);
	connect(sockfd, (sockaddr*)&TM_Addr, sizeof(TM_Addr));
		//error check
	
	char buf_T[1024] = "", tmp[1024] = "";
	while(1)
	{
		recv(sockfd, buf_T, 1024, 0);
		if(buf_T[0] != '3')
		{
			strcpy(buf_T, "");
			continue;
		}
		sscanf(buf_T + 2 * sizeof(char), "%s %d %s", Program_Path, &Device_Parameter, Input_Path);
		printf("Program Path: %s\nDevice_Parameter: %d\nInput Path: %s\n", Program_Path, Device_Parameter, Input_Path);
		CPU_Task_num = 0;
		GPU_Task_num = 0;
		for(iter = Node_Pool.begin(); iter != Node_Pool.end(); iter++)
		{
			CPU_Task_num += iter->second.CPU_num;
			GPU_Task_num += iter->second.GPU_num;
		}
		strcpy(buf_T, "4 ");
		sprintf(tmp, "%d %d", CPU_Task_num, GPU_Task_num);
		strcat(buf_T, tmp);
		send(sockfd, buf_T, 1024, 0);
		strcpy(buf_T, "");
	}

}
