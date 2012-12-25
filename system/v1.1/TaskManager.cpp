/*
 * This file contains functions of TaskManager.
 *
 * by Qichen Pan.
 */

#include "lib.h"
#include "util.h"

int TM_RM_Port = 0, TM_Task_Port = 0, TM_IF_Port = 0;
char TM_IP[80] = "";
char Program_Path[512] = "";
char Input_Path[512] = "";
int Device_Parameter = -1;
int CPU_Task_num = -1, GPU_Task_num = -1;
bool Splits_Set = false, Task_All_Connected = false;
pthread_cond_t Job_Complete_IF = PTHREAD_COND_INITIALIZER;
pthread_cond_t Job_Complete_RM = PTHREAD_COND_INITIALIZER;
pthread_mutex_t RM_Mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t IF_Mutex = PTHREAD_MUTEX_INITIALIZER;

vector<Split> Unassigned_Split;
map<int, Split> Ongoing_Split;
vector<Split> Finished_Split;
map<int, Task_Info> CPU_Task_Info;
map<int, Task_Info> GPU_Task_Info;

void* conn_to_IF(void*);
void* conn_to_RM(void*);
void* conn_to_Task(void*);
int Set_Splits(const char*);

int main()
{
	// Get TM information
	FILE* TM_Info = fopen("TM_Info", "r");
	fscanf(TM_Info, "%s", TM_IP);
	fscanf(TM_Info, "%d", &TM_RM_Port);
	fscanf(TM_Info, "%d", &TM_Task_Port);
	fscanf(TM_Info, "%d", &TM_IF_Port);
	fclose(TM_Info);

	// Create thread to deal with IF
	pthread_t thread_conn_to_IF;
	pthread_create(&thread_conn_to_IF, NULL, conn_to_IF, NULL);

	// Create thread to deal with RM
	pthread_t thread_conn_to_RM;
	pthread_create(&thread_conn_to_RM, NULL, conn_to_RM, NULL);

	// Create thread to deal with Tasks using Epoll
	pthread_t thread_conn_to_Task;
	pthread_create(&thread_conn_to_Task, NULL, conn_to_Task, NULL);

	pthread_join(thread_conn_to_IF, NULL);
	pthread_join(thread_conn_to_RM, NULL);
	pthread_join(thread_conn_to_Task, NULL);
	return 0;
}

void* conn_to_RM(void* args)
{
	int sockfd_RM;
	sockfd_RM = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd_RM < 0)
	{
		perror("Error occurs opening socket.\n");
		exit(1);
	}

	struct sockaddr_in TM_RM_Addr;
	TM_RM_Addr.sin_family = AF_INET;
	TM_RM_Addr.sin_addr.s_addr = htonl(INADDR_ANY);
	TM_RM_Addr.sin_port = htons(TM_RM_Port);
	bind(sockfd_RM, (struct sockaddr*)& TM_RM_Addr, sizeof(TM_RM_Addr));
	listen(sockfd_RM, 5);

	int RM_Client;
	struct sockaddr_in RM_Addr;
	socklen_t RM_Sock_Len = sizeof(sockaddr_in);
	RM_Client = accept(sockfd_RM, (struct sockaddr*)&RM_Addr, &RM_Sock_Len);

	while(1)
	{
		if(strcmp(Program_Path, "") != 0)
		{
			char tmp[1024] = "";
			sprintf(tmp, "3 %s %d %s", Program_Path, Device_Parameter, Input_Path);
			send(RM_Client, tmp, 1024, 0);
			recv(RM_Client, tmp, 1024, 0);
			if(tmp[0] != '4')
			{
				continue;
			}
			sscanf(tmp + 2 * sizeof(char), "%d %d", &CPU_Task_num, &GPU_Task_num);
			printf("Resources:\nCPU_Task_num: %d\nGPU_Task_num: %d\n", CPU_Task_num, GPU_Task_num);
			
			// Set splits from input file
			if(Set_Splits(Input_Path) == -1)
			{
				printf("Error occurs when setting splits.\n");
				exit(1);
			}
			Splits_Set = true;
			printf("All splits are set!\n");
			
			pthread_mutex_lock(&RM_Mutex);
			pthread_cond_wait(&Job_Complete_RM, &RM_Mutex);
			pthread_mutex_unlock(&RM_Mutex);
		}
		sleep(1);
	}
	return NULL;
}

void* conn_to_IF(void* args)
{
	int sockfd_IF;
	char tmp[1024] = "";
	sockfd_IF = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd_IF < 0)
	{
		perror("Error occurs opening socket.\n");
		exit(1);
	}

	struct sockaddr_in TM_IF_Addr;
	TM_IF_Addr.sin_family = AF_INET;
	TM_IF_Addr.sin_addr.s_addr = htonl(INADDR_ANY);
	TM_IF_Addr.sin_port = htons(TM_IF_Port);
	bind(sockfd_IF, (struct sockaddr*)& TM_IF_Addr, sizeof(TM_IF_Addr));
	while(1)
	{
		listen(sockfd_IF, 5);

		int IF_Client;
		struct sockaddr_in IF_Addr;
		socklen_t IF_Sock_Len = sizeof(sockaddr_in);
		IF_Client = accept(sockfd_IF, (struct sockaddr*)&IF_Addr, &IF_Sock_Len);
		if(IF_Client == -1)
		{
			perror("Error occurs when accepting IF client.\n");
			exit(1);
		}

		recv(IF_Client, tmp, 1024, 0);
		if(tmp[0] != '2')
		{
			continue;
		}
		sscanf(tmp + sizeof(char) * 2, "%s %d %s", Program_Path, &Device_Parameter, Input_Path);
		printf("New job comes!\nProgram_Path: %s\nDevice_Parameter: %d\nInput_Path: %s\n", Program_Path, Device_Parameter, Input_Path);

		/* complete task */
		pthread_mutex_lock(&IF_Mutex);
		pthread_cond_wait(&Job_Complete_IF, &IF_Mutex);
		pthread_mutex_unlock(&IF_Mutex);
		sprintf(tmp, "b Complete");
		send(IF_Client, tmp, 1024, 0);
		printf("Job is now done.\n");
		/* clear Paths Parameters and vectors maps*/
		strcpy(Program_Path, "");
		strcpy(Input_Path, "");
		Device_Parameter = -1;
		CPU_Task_num = -1;
		GPU_Task_num = -1;
		Splits_Set = false;
		Task_All_Connected = false;
		Unassigned_Split.clear();
		Ongoing_Split.clear();
		Finished_Split.clear();
		CPU_Task_Info.clear();
		GPU_Task_Info.clear();

		
		
	}
	return NULL;
}

void* conn_to_Task(void* args)
{
	const int MAX_TASK_NUM = 256;
	int listenfd, connfd, tmpfd, epfd, nfds, i;
	char buf_in[1024], buf_out[1024];
	map<int, Split>::iterator Split_Map_iter;
	vector<Split>::iterator Split_Vector_iter;
	list<Split>::iterator Split_List_iter;
	map<int, Task_Info>::iterator Task_iter;
	socklen_t Task_Len;

	struct epoll_event ev, events[MAX_TASK_NUM];
	epfd = epoll_create(MAX_TASK_NUM);
	struct sockaddr_in TM_Addr, Task_Addr;
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	
	ev.data.fd = listenfd;
	ev.events = EPOLLIN;

	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
	bzero(&TM_Addr, sizeof(TM_Addr));
	TM_Addr.sin_family = AF_INET;
	TM_Addr.sin_addr.s_addr = htonl(INADDR_ANY);
	TM_Addr.sin_port = htons(TM_Task_Port);

	bind(listenfd, (sockaddr*)&TM_Addr, sizeof(TM_Addr));
	listen(listenfd, MAX_TASK_NUM);

	while(1)
	{
		nfds = epoll_wait(epfd, events, 256, 500);

		for(i = 0; i < nfds; i++)
		{
			if(events[i].data.fd == listenfd)//A new Task connecting to TM
			{
				connfd = accept(listenfd, (sockaddr*)&Task_Addr, &Task_Len);
				if(connfd < 0)
				{
					perror("Error occurs when Task wants to connect to T<.\n");
					exit(1);
				}

				ev.data.fd = connfd;
				ev.events = EPOLLIN;
				
				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
			}else if(events[i].events & EPOLLIN)// Task wants to send something
			{
				tmpfd = events[i].data.fd;
				if(tmpfd < 0)
				{
					epoll_ctl(epfd, EPOLL_CTL_DEL, tmpfd, &ev);
					continue;
				}
				if(recv(tmpfd, buf_in, 1024, 0) < 0)
				{
					printf("Error occurs when fetching from Task with ID: %d\n", events[i].data.fd);
					continue;
				}

				if(buf_in[0] == '8')
				{
					// Deal with Task initial
					char Task_Kind[10] = "";
					sscanf(buf_in + sizeof(char) * 2, "%s", Task_Kind);
					if(strcmp(Task_Kind, "CPU") == 0)
					{	
						Task_Info new_Task;
						CPU_Task_Info.insert(pair<int, Task_Info>(events[i].data.fd, new_Task));
						printf("A new CPU task connected with ID: %d.\n", events[i].data.fd);
					}else if(strcmp(Task_Kind, "GPU") == 0)
					{
						Task_Info new_Task;
						GPU_Task_Info.insert(pair<int, Task_Info>(events[i].data.fd, new_Task));
						printf("A new GPU task connected with ID: %d.\n", events[i].data.fd);
					}else
					{
						printf("Invalid MSG from Task with ID: %d.\n", events[i].data.fd);
						continue;
					}
					if(CPU_Task_Info.size() == CPU_Task_num && GPU_Task_Info.size() == GPU_Task_num)
					{
						printf("All tasks are connected!\n");
						Task_All_Connected = true;
					}
				}
				else if(buf_in[0] == '9')
				{
					int id = 0;
					long t = 0;
					sscanf(buf_in + sizeof(char) * 2, "%d %ld", &id, &t);
					
					//printf("This id: %d\n", id);
					//for(Split_Map_iter = Ongoing_Split.begin(); Split_Map_iter != Ongoing_Split.end(); Split_Map_iter++)
					//{
					//	printf("%d %d\n", Split_Map_iter->first, Split_Map_iter->second.Split_Id);
					//}
					//printf("%d\n", Unassigned_Split.size());
					
					// Search the Split in Ongoing Map
					Split_Map_iter = Ongoing_Split.find(id);
					if(Split_Map_iter == Ongoing_Split.end())
					{
						//printf("Error: no such Split ongoing!\n");

						epoll_ctl(epfd, EPOLL_CTL_DEL, tmpfd, &ev);
						continue;
					}		

					// Put it into Finished vector
					Split tmp_Split(Split_Map_iter->second);
					tmp_Split.Finished_Time = t;
					Finished_Split.push_back(tmp_Split);

					// Erase it from Ongoing Map
					Ongoing_Split.erase(Split_Map_iter);
					
					// judge if it is a CPU Task
					Task_iter = CPU_Task_Info.find(events[i].data.fd);
					Task_Info *tmp_Task;
					if(Task_iter == CPU_Task_Info.end())
					{
						Task_iter = GPU_Task_Info.find(events[i].data.fd);
						if(Task_iter == GPU_Task_Info.end())
						{
							printf("Error: no such Task can be found with ID: %d.\n", events[i].data.fd);
							continue;
						}
					}
					tmp_Task = &(Task_iter->second);
					
					// revise information in that Task_Info
					Split *new_Split = new Split(tmp_Task->Task_Queue.front());
					new_Split->Finished_Time = t;
					tmp_Task->Splits_Finish_Time.push_back(*new_Split);
					tmp_Task->Task_Queue.pop();
					printf("Split %d is done at %ld by Task %d.\n", new_Split->Split_Id, new_Split->Finished_Time, events[i].data.fd);

					// To see if Job is complete
					printf("Uassigned_Split: %d\nOngoing_Split: %d\n", Unassigned_Split.size(), Ongoing_Split.size());
					if(Unassigned_Split.empty()  && Ongoing_Split.empty())
					{
						pthread_mutex_lock(&RM_Mutex);
						pthread_cond_signal(&Job_Complete_RM);
						pthread_mutex_unlock(&RM_Mutex);
						pthread_mutex_lock(&IF_Mutex);
						pthread_cond_signal(&Job_Complete_IF);
						pthread_mutex_unlock(&IF_Mutex);
					}
				}else if(buf_in[0] == 'd')
				{
				}else
				{
					printf("%s\nInvalid MSG from Task with ID: %d.\n",buf_in,  events[i].data.fd);
					continue;
				}

				ev.data.fd = tmpfd;
				ev.events = EPOLLOUT;
				epoll_ctl(epfd, EPOLL_CTL_MOD, tmpfd, &ev);
			}else if(events[i].events & EPOLLOUT)// Task wants to fetch something
			{
				tmpfd = events[i].data.fd;
				// To see if there is no unassigned splits
				if(Unassigned_Split.empty())
				{
					sprintf(buf_out, "a Finished");
					send(tmpfd, buf_out, 1024, 0);
					strcpy(buf_out, "");
				}else
				{
					// To see if Tasks are all connected and Splits are all set
					if(Splits_Set == false || Task_All_Connected == false)
					{
						continue;
					}
					
					// Pop Split from unassigned vector and put it into Ongoing map
					Split tmp_Split = Unassigned_Split.back();
					Unassigned_Split.pop_back();
					Ongoing_Split.insert(pair<int, Split>(tmp_Split.Split_Id, tmp_Split));

					// Judge if it is a CPU task
					Task_iter = CPU_Task_Info.find(events[i].data.fd);
					Task_Info *tmp_Task;
					if(Task_iter == CPU_Task_Info.end())
					{
						Task_iter = GPU_Task_Info.find(events[i].data.fd);
						if(Task_iter == GPU_Task_Info.end())
						{
							printf("Error: no such Task can be found with ID: %d.\n", events[i].data.fd);
							continue;
						}
					}
					tmp_Task = &(Task_iter->second);
					
					// Update Task_Info
					tmp_Task->Task_Queue.push(tmp_Split);

					// Send to Task
					sprintf(buf_out, "c %d %ld %ld", tmp_Split.Split_Id, tmp_Split.begin, tmp_Split.offset);

					printf("Send a new Split to Task %d, Split_Id: %d, begin: %ld, offset: %ld.\n", events[i].data.fd, tmp_Split.Split_Id, tmp_Split.begin, tmp_Split.offset);
					send(tmpfd, buf_out, 1024, 0);
					strcpy(buf_out, "");
				}

				ev.data.fd = tmpfd;
				ev.events = EPOLLIN;
				epoll_ctl(epfd, EPOLL_CTL_MOD, tmpfd, &ev);
			}else
			{
				continue;
				// do nothing
			}
		}
	}
	return NULL;
}

int Set_Splits(const char* Input_Path)
{
	FILE* Input = fopen(Input_Path, "r");
	if(Input == NULL)
	{
		printf("Error when opening input file.\n");
		return -1;
	}
	fseek(Input, 0l, SEEK_END);
	long input_len = ftell(Input);
	int Split_Num = (input_len - 1) / 67108864 + 1;
	int i = 0;
	for(i = 0; i < Split_Num; i++)
	{
		Split tmp_Split(i, i * 67108864, 67108864);
		if(i == Split_Num - 1)
		{
			tmp_Split.offset = input_len - (Split_Num - 1) * 67108864;
		}
		Unassigned_Split.push_back(tmp_Split);
	}
	vector<Split>::iterator iter;
	for(iter = Unassigned_Split.begin(); iter != Unassigned_Split.end(); iter++)
	{
		printf("Split_ID: %d\tBegin: %ld\n", iter->Split_Id, iter->begin);
	}
	
	return 0;
}
