#include "syscall.h"

int
main()
{
	char* name = "/home/li/test";
	char* b = "/home/li/test2";
    int id = Open(name);
    char* buf = "1234567890";
    Read(buf, 5, id);
    Close(id);
    Create(b);
    id = Open(b);
    Write(buf, 5, id);
    Close(id);
    Exit(0);
}
