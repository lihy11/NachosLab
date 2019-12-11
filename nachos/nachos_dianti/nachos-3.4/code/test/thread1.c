#include "syscall.h"

int
main()
{
	char* name = "/home/li/thread2";
	int exitCode = -2;
    SpaceId id = Exec(name);
    exitCode = Join(id);
    Exit(exitCode);
}
