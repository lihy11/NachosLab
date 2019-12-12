#include "syscall.h"

int all[3] = {1,2,3};
int i,j;
void fork(){
	for(i = 0; i < 2; i ++){
		all[i] = all[i] * all[i+1];
	}
	Exit(all[1]);
}
int
main()
{

	for(j = 0; j < 2; j ++){
		all[j] = all[j] + all[j+1];
	}
	Fork(fork);
	Yield();
	Exit(all[0]);
}
