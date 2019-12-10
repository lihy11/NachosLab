// exception.cc
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

///*  for coding */
#include "machine.h"
#include "filesys.h"
extern Machine *machine;
extern FileSystem* fileSystem;
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2.
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions
//	are in machine.h.
//----------------------------------------------------------------------
int translateAddr(int vaddr) {
	int faddr = 0;
	machine->ReadMem(vaddr, 4, &faddr);
	machine->Translate(vaddr, &faddr, 4, FALSE);
	printf("translating vaddr %x to faddr %x\n", vaddr,
			machine->mainMemory + faddr);
	return machine->mainMemory + faddr;
}
void SyscallHandler(int type) {
	switch (type) {
	case SC_Halt: {
		DEBUG('a', "Shutdown, initiated by user program.\n");
		interrupt->Halt();
		break;
	}
	case SC_Exit: {
		DEBUG('a', "Exit, exit the user prog.\n");
		printf("Exit code is %d\n", machine->ReadRegister(4));
		currentThread->Finish();
		break;
	}
	case SC_Exec: {
		break;
	}
	case SC_Fork: {
		break;
	}
	case SC_Yield: {
		break;
	}
	case SC_Join: {
		break;
	}
	case SC_Create: {
		char* name = (char*) translateAddr(machine->ReadRegister(4));
		fileSystem->Create(name, 0, TRUE);
//		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		break;
	}
	case SC_Open: {
		char* name = (char*) translateAddr(machine->ReadRegister(4));
		OpenFile* openfile = fileSystem->Open(name);
		machine->WriteRegister(2, (int) openfile);
//		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		break;
	}
	case SC_Close: {
		int fileAddr = machine->ReadRegister(4);
		fileSystem->Close((OpenFile*) fileAddr);
//		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		break;
	}
	case SC_Write: {
		char* name = (char*) translateAddr(machine->ReadRegister(4));
		int size = machine->ReadRegister(5);
		OpenFile* file = (OpenFile*) machine->ReadRegister(6);

		fileSystem->fwrite(file, name, size);
//		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		break;
	}
	case SC_Read: {
		char* name = (char*) translateAddr(machine->ReadRegister(4));
		int size = machine->ReadRegister(5);
		OpenFile* file = (OpenFile*) machine->ReadRegister(6);

		int num = fileSystem->fread(file, name, size);
		machine->WriteRegister(2, num);
//		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		break;
	}
	}

}
void ExceptionHandler(ExceptionType which) {
	int type = machine->ReadRegister(2);

	if (which == SyscallException) {
		return SyscallHandler(type);
	} else if (which == PageFaultException) {
		/*
		 虚拟内存缺页中断。
		 1.对于TLB， 用需要替换的地址替换TLB中现有的某一项，应该查询页表找到对应项，再利用选择算法替换某一项
		 2.对于页表，需要将对应的磁盘数据读取至内存，为其分配一个物理内存，并在页表中添加其转换关系。
		 */
		int badAddr = machine->ReadRegister(BadVAddrReg);
		DEBUG('a', "Page fault exception of addr %x.\n", badAddr);
		if (machine->tlb != NULL) { //使用tlb
			machine->replaceTlb(badAddr);
		} else {
			machine->replacePageTable(badAddr);
		}
	} else {
		printf("Unexpected user mode exception %d %d\n", which, type);
		ASSERT(FALSE);
	}
}
