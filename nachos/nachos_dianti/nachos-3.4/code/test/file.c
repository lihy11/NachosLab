#include "syscall.h"

int
main()
{
	char* name = "/home/li/test";
    int id = Open(name);
    char* buf = "1234567890";
    Read(buf, 5, id);
    Close(id);
    Exit((int)buf);
//
//    char* b = "/home/li/test2";
//    id = Open(b);
//    Write(buf, 5, id);
//    Close(id);
}
