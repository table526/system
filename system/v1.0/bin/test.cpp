#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <iostream>
using namespace std;

int main()
{
	FILE* f = fopen("Node_Info", "r");
	char buf[1024] = "";
	char tmp[20] = "";
	int j = 0;
	long len;
	fseek(f, 0L, SEEK_END);
	len = ftell(f);
	fseek(f, 0L, SEEK_SET);
	while(ftell(f) < len)
	{
	//	fscanf(f, "%d", &j);
	//	sprintf(tmp, "%d ", j);
	//	strcat(buf, tmp);
		fscanf(f, "%c", buf);
	}
	printf("%s", buf);
	fclose(f);
	return 0;
}
