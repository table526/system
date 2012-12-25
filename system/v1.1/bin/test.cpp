#include "lib.h"

int main(int argc, char* argv[])
{
	int pid = fork();
	int status;
	time_t t = time(NULL);
	char Program_Path[512] = "", Input_Path[512] = "", begin_char[20] ="", offset_char[20] = "";
	strcpy(Program_Path, argv[1]);
	strcpy(Input_Path, argv[2]);
	strcpy(begin_char, argv[3]);
	strcpy(offset_char, argv[4]);
	printf("begin : %ld\n", t);
	if(pid == 0)
	{
		execl(Program_Path, Program_Path, Input_Path, begin_char, offset_char, (char*)0);
	//	execl("/home/pan/mmds/proj/2/print",  "","/home/pan/mmds/proj/2/HSV64.txt", "0", "20000000", (char*)0);
	//	execl("/home/pan/system/v1.1/bin/TASK", "TASK", "CPU", "1", "/home/pan/mmds/proj/2/print", "/home/pan/mmds/proj/2/HSV64.txt", (char*)0);
	//	execl("./TASK", "TASK", "CPU", "1", "/home/pan/mmds/proj/2/print", "/home/pan/mmds/proj/HSV64.txt", NULL);
	}else
	{
		wait(&status);
	}
	t = time(NULL);
	printf("end: %ld\n", t);
	return 0;
}

