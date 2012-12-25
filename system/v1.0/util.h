/*
 *	This file contains several structures and classes used in our system,
 *	including Node, Split, Task_Info.
 *
 *	by Qichen Pan.
 */

#ifndef _UTIL_H_
#define _UTIL_H_
#include "lib.h"
using namespace std;

class Node
{
	public:
		int CPU_num;
		int GPU_num;
		int *CPU_Core_num;
		int *GPU_Model;

		Node(){CPU_num = 0; GPU_num = 0;}
		Node(int c, int g)
		{
			CPU_num = c;
			GPU_num = g;
			CPU_Core_num = (int*)malloc(sizeof(int) * CPU_num);
			GPU_Model = (int*)malloc(sizeof(int) * GPU_num);
		}
		~Node(){free(CPU_Core_num); free(GPU_Model);}

};

class Split
{
	public:
		int Split_Id;
		time_t	Finished_Time;
		long begin;
		long offset;
		
		Split(){}
		Split(int i)
		{
			Split_Id = i;
		}
		Split(int i, long lr, long os)
		{
			Split_Id = i;
			begin = lr; 
			offset = os;
			Finished_Time = -1;
		}
		~Split(){}
};

class Task_Info
{
	public:
		queue<Split> TM_Task_Queue;
		list<Split> Splits_Finish_Time;

		Task_Info(){}
		~Task_Info(){}
};

#endif

