#include "syscall.h"

int all = 0;
int i,j;
void fork(){
	for(i = 0; i < 2; i ++){
		all ++;
		Yield();
	}
	Exit(0);
}
int
main()
{
	Fork(fork);
	Yield();
	for(j = 0; j < 2; j ++){
		all = all * 2;
		Yield();
	}
	Exit(all);
}
