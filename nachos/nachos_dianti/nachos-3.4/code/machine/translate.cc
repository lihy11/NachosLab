// translate.cc
//	Routines to translate virtual addresses to physical addresses.
//	Software sets up a table of legal translations.  We look up
//	in the table on every memory reference to find the true physical
//	memory location.
//
// Two types of translation are supported here.
//
//	Linear page table -- the virtual page # is used as an index
//	into the table, to find the physical page #.
//
//	Translation lookaside buffer -- associative lookup in the table
//	to find an entry with the same virtual page #.  If found,
//	this entry is used for the translation.
//	If not, it traps to software with an exception.
//
//	In practice, the TLB is much smaller than the amount of physical
//	memory (16 entries is common on a machine that has 1000's of
//	pages).  Thus, there must also be a backup translation scheme
//	(such as page tables), but the hardware doesn't need to know
//	anything at all about that.
//
//	Note that the contents of the TLB are specific to an address space.
//	If the address space changes, so does the contents of the TLB!
//
// DO NOT CHANGE -- part of the machine emulation
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "machine.h"
#include "addrspace.h"
#include "system.h"

// Routines for converting Words and Short Words to and from the
// simulated machine's format of little endian.  These end up
// being NOPs when the host machine is also little endian (DEC and Intel).

unsigned int
WordToHost(unsigned int word)
{
#ifdef HOST_IS_BIG_ENDIAN
	register unsigned long result;
	result = (word >> 24) & 0x000000ff;
	result |= (word >> 8) & 0x0000ff00;
	result |= (word << 8) & 0x00ff0000;
	result |= (word << 24) & 0xff000000;
	return result;
#else
	return word;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned short
ShortToHost(unsigned short shortword)
{
#ifdef HOST_IS_BIG_ENDIAN
	register unsigned short result;
	result = (shortword << 8) & 0xff00;
	result |= (shortword >> 8) & 0x00ff;
	return result;
#else
	return shortword;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned int
WordToMachine(unsigned int word) { return WordToHost(word); }

unsigned short
ShortToMachine(unsigned short shortword) { return ShortToHost(shortword); }

//----------------------------------------------------------------------
// Machine::ReadMem
//      Read "size" (1, 2, or 4) bytes of virtual memory at "addr" into
//	the location pointed to by "value".
//
//   	Returns FALSE if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to read from
//	"size" -- the number of bytes to read (1, 2, or 4)
//	"value" -- the place to write the result
//----------------------------------------------------------------------

bool Machine::ReadMem(int addr, int size, int *value)
{
	int data;
	ExceptionType exception;
	int physicalAddress;

	DEBUG('a', "Reading VA 0x%x, size %d\n", addr, size);

	exception = Translate(addr, &physicalAddress, size, FALSE);
	if (exception != NoException)
	{
		machine->RaiseException(exception, addr);
		return FALSE;
	}
	switch (size)
	{
	case 1:
		data = machine->mainMemory[physicalAddress];
		*value = data;
		break;

	case 2:
		data = *(unsigned short *)&machine->mainMemory[physicalAddress];
		*value = ShortToHost(data);
		break;

	case 4:
		data = *(unsigned int *)&machine->mainMemory[physicalAddress];
		*value = WordToHost(data);
		break;

	default:
		ASSERT(FALSE);
	}

	DEBUG('a', "\tvalue read = %8.8x\n", *value);
	return (TRUE);
}

//----------------------------------------------------------------------
// Machine::WriteMem
//      Write "size" (1, 2, or 4) bytes of the contents of "value" into
//	virtual memory at location "addr".
//
//   	Returns FALSE if the translation step from virtual to physical memory
//   	failed.
//
//	"addr" -- the virtual address to write to
//	"size" -- the number of bytes to be written (1, 2, or 4)
//	"value" -- the data to be written
//----------------------------------------------------------------------

bool Machine::WriteMem(int addr, int size, int value)
{
	ExceptionType exception;
	int physicalAddress;

	DEBUG('a', "Writing VA 0x%x, size %d, value 0x%x\n", addr, size, value);

	exception = Translate(addr, &physicalAddress, size, TRUE);
	if (exception != NoException)
	{
		machine->RaiseException(exception, addr);
		return FALSE;
	}
	switch (size)
	{
	case 1:
		machine->mainMemory[physicalAddress] = (unsigned char)(value & 0xff);
		break;

	case 2:
		*(unsigned short *)&machine->mainMemory[physicalAddress] = ShortToMachine((unsigned short)(value & 0xffff));
		break;

	case 4:
		*(unsigned int *)&machine->mainMemory[physicalAddress] = WordToMachine((unsigned int)value);
		break;

	default:
		ASSERT(FALSE);
	}

	return TRUE;
}

//----------------------------------------------------------------------
// Machine::Translate
// 	Translate a virtual address into a physical address, using
//	either a page table or a TLB.  Check for alignment and all sorts
//	of other errors, and if everything is ok, set the use/dirty bits in
//	the translation table entry, and store the translated physical
//	address in "physAddr".  If there was an error, returns the type
//	of the exception.
//
//	"virtAddr" -- the virtual address to translate
//	"physAddr" -- the place to store the physical address
//	"size" -- the amount of memory being read or written
// 	"writing" -- if TRUE, check the "read-only" bit in the TLB
//----------------------------------------------------------------------

ExceptionType
Machine::Translate(int virtAddr, int *physAddr, int size, bool writing)
{

	int i;
	unsigned int vpn, offset;
	TranslationEntry *entry;
	unsigned int pageFrame;
	ExceptionType exception = NoException;

	DEBUG('a', "\tTranslate 0x%x, %s: ", virtAddr, writing ? "write" : "read");

	// check for alignment errors
	if (((size == 4) && (virtAddr & 0x3)) || ((size == 2) && (virtAddr & 0x1)))
	{
		DEBUG('a', "alignment problem at %d, size %d!\n", virtAddr, size);
		return AddressErrorException;
	}

	// we must have either a TLB or a page table, but not both!
	ASSERT(tlb == NULL || pageTable == NULL);
	ASSERT(tlb != NULL || pageTable != NULL);

	// calculate the virtual page number, and offset within the page,
	// from the virtual address
	vpn = (unsigned)virtAddr / PageSize;
	offset = (unsigned)virtAddr % PageSize;

	if (tlb == NULL)
	{
		entry = translatePageTable(vpn, offset, &exception);
	}
	else
	{
		entry = translateTlb(vpn, offset, &exception);
	}

	if (exception != NoException)
	{
		return exception;
	}

	if (entry->readOnly && writing)
	{ // trying to write to a read-only page
		DEBUG('a', "%d mapped read-only at %d in TLB!\n", virtAddr, i);
		return ReadOnlyException;
	}
	pageFrame = entry->physicalPage;

	// if the pageFrame is too big, there is something really wrong!
	// An invalid translation was loaded into the page table or TLB.
	if (pageFrame >= NumPhysPages)
	{
		DEBUG('a', "*** frame %d > %d!\n", pageFrame, NumPhysPages);
		return BusErrorException;
	}
	entry->use = TRUE; // set the use, dirty bits
	if (writing)
		entry->dirty = TRUE;
	*physAddr = pageFrame * PageSize + offset;
	ASSERT((*physAddr >= 0) && ((*physAddr + size) <= MemorySize));
	DEBUG('a', "phys addr = 0x%x\n", *physAddr);
	return NoException;
}

TranslationEntry *
Machine::translateTlb(int vpn, int offset, ExceptionType *exception)
{
	int i = 0;
	TranslationEntry *entry;
	for (entry = NULL, i = 0; i < TLBSize; i++)
	{
		if (tlb[i].valid && (tlb[i].virtualPage == vpn))
		{
			entry = &tlb[i]; // FOUND!
			if (replaceMethod == 1)
			{ //LRU
				entry->count = 0;
			}
		}
		else
		{
			entry->count++;
		}
	}
	if (entry == NULL)
	{ // not found
		DEBUG('a', "*** no valid TLB entry found for this virtual page!\n");
		*exception = PageFaultException; // really, this is a TLB fault,
										 // the page may be in memory,
										 // but not in the TLB
	}
	return entry;
}

/*
*/
TranslationEntry *
Machine::translatePageTable(int vpn, int offset, ExceptionType *exception)
{
	unsigned int virtAddr = vpn * PageSize + offset;
	TranslationEntry *entry;
	// => page table => vpn is index into table
	if (vpn >= pageTableSize)
	{
		DEBUG('a', "virtual page # %d too large for page table size %d!\n",
			  virtAddr, pageTableSize);
		*exception = AddressErrorException;
		return entry;
	}
	else if (!pageTable[vpn].valid)
	{
		DEBUG('a', "virtual page # %d too large for page table size %d!\n",
			  virtAddr, pageTableSize);
		*exception = PageFaultException;
		return entry;
	}
	entry = &pageTable[vpn];
	return entry;
}

///*
//倒排页表查找算法
//*/
//TranslationEntry*
//Machine::translatePageTableDes(int vpn, int offset, ExceptionType *exception ){
//	unsigned int virtAddr = vpn * PageSize + offset;
//	TranslationEntry *entry;
//	// => page table => vpn is index into table
//	if (vpn >= pageTableSize)
//	{
//		DEBUG('a', "virtual page # %d too large for page table size %d!\n",
//			  virtAddr, pageTableSize);
//		*exception = AddressErrorException;
//		return entry;
//	}
//	for(int i = 0; i < tableSize; i ++){
//		if(pageTable[i].valid && pageTable[i].virtualPage == vpn){
//			entry = &pageTable[i];
//		}
//	}
//	if(entry == NULL){
//		*exception = PageFaultException;
//	}
//	return entry;
//}
/*
	@author lihaiyang
	1. 查找页表，找到对应的页表项
	2. 选择一个tlb项进行替换
*/
ExceptionType
Machine::replaceTlb(int virtAddr)
{
	unsigned int vpn, offset;
	TranslationEntry *entry;

	vpn = (unsigned)virtAddr / PageSize;
	offset = (unsigned)virtAddr % PageSize;

	if (vpn >= pageTableSize)
	{
		DEBUG('a', "virtual page # %d too large for page table size %d!\n",
			  virtAddr, pageTableSize);
		return AddressErrorException;
	}
	else if (!pageTable[vpn].valid)
	{
		DEBUG('a', "virtual page # %d not valid\n", virtAddr);
		return PageFaultException;
	}
	entry = &pageTable[vpn];

	TranslationEntry *replaceEntry = selectOne(tlb, TLBSize);

	*replaceEntry = *entry;

	return NoException;
}

/*
	@author lihaiyang
	1. virtAddr位置未分配物理页面，选择一个空闲的物理页面分配，如果物理页面不足，
		则选择一个物理页面替换掉（当前直接报错）
	2. virtAddr位置的物理页面没有在内存中，需要从磁盘调入物理页面，
		entry中应该存放了物理磁盘的地址（此处用内存模拟磁盘，磁盘地址即为某个内存地址）
	解决：在entry中加入标识位，on disk，表示当前页表缓存在磁盘上，diskAddr表示当前页面在磁盘的位置
	在machine中创建虚拟的内存空间disk来模拟磁盘，为列表形式，忽略磁盘的数据管理等操作。
*/
ExceptionType
Machine::replacePageTable(int virtAddr)
{
	unsigned int vpn, offset;
	TranslationEntry *entry;

	vpn = (unsigned)virtAddr / PageSize;
	offset = (unsigned)virtAddr % PageSize;

	if (vpn >= pageTableSize)
	{
		DEBUG('a', "virtual page # %d too large for page table size %d!\n",
			  virtAddr, pageTableSize);
		return AddressErrorException;
	}

	int *pageFromDisk = NULL;
	if (pageTable[vpn].onDisk)
	{ //页面在磁盘，从磁盘重新读取页面到pageFromDisk
		pageFromDisk = (int*)pageTable[vpn].diskAddr;
	}
	//分配一个物理页面，并更新页表
	int pageNO = findNullPyhPage();
	if (pageNO == -1)
	{ //物理页面满

		TranslationEntry *replacePage = selectOne(pageTable, pageTableSize); //寻找替换页面

		int *diskPage = new int[PageSize / 4]; //将数据拷贝存储到磁盘
		memcpy(diskPage, mainMemory + replacePage->physicalPage * PageSize, PageSize);
		disk->Append((void *)diskPage);

		replacePage->onDisk = true; //更新替换页表项
		replacePage->diskAddr = (int)diskPage;
		pageNO = replacePage->physicalPage;
	}
	pageTable[vpn].count = 0; //更新当前页表项
	pageTable[vpn].physicalPage = pageNO;
	pageTable[vpn].valid = true;

	if (pageFromDisk != NULL)
	{ //拷贝磁盘数据
		memcpy(mainMemory + pageNO * PageSize, pageFromDisk, PageSize);
	}
}
///*
//倒排也表缺页异常处理算法
//*/
//ExceptionType
//Machine::replacePageTableDes(int virtAddr, List* pageTable)
//{
//	unsigned int vpn, offset;
//	TranslationEntry *entry;
//
//	vpn = (unsigned)virtAddr / PageSize;
//	offset = (unsigned)virtAddr % PageSize;
//
//	if (vpn >= pageTableSize)
//	{
//		DEBUG('a', "virtual page # %d too large for page table size %d!\n",
//			  virtAddr, pageTableSize);
//		return AddressErrorException;
//	}
//	/* 遍历页表 */
//	for((ListElement*) entry=pageTable->getHead(); entry != NULL; entry = (ListElement*)entry->next){
//
//	}
//
//	int *pageFromDisk = NULL;
//	if (pageTable[vpn].onDisk)
//	{ //页面在磁盘，从磁盘重新读取页面到pageFromDisk
//		for (ListElement *page = disk->getHead(); page != NULL; page = page->next)
//		{
//			if ((int)page == pageTable[vpn].diskAddr)
//			{
//				pageFromDisk = (int *)page;
//				break;
//			}
//		}
//	}
//	//分配一个物理页面，并更新页表
//	int pageNO = findNullPyhPage();
//	if (pageNO == -1)
//	{ //物理页面满
//
//		TranslationEntry *replacePage = selectOne(pageTable, pageTableSize); //寻找替换页面
//
//		int *diskPage = new int[PageSize / 4]; //将数据拷贝存储到磁盘
//		memcpy(diskPage, mainMemory + replacePage->physicalPage * PageSize, PageSize);
//		disk->Append((void *)diskPage);
//
//		replacePage->onDisk = true; //更新替换页表项
//		replacePage->diskAddr = (int)diskPage;
//		pageNO = replacePage->physicalPage;
//	}
//	pageTable[vpn].count = 0; //更新当前页表项
//	pageTable[vpn].physicalPage = pageNO;
//	pageTable[vpn].valid = true;
//
//	if (pageFromDisk != NULL)
//	{ //拷贝磁盘数据
//		memcpy(mainMemory + pageNO * PageSize, pageFromDisk, PageSize);
//	}
//}

/*
	@author lihaiyang
	在指定的列表中选择一个条目，可以使用不同的算法
	1. LRU, TranslationEntry 条目中的count变量记录了条目的上一次使用离现在有多久，每当
		一个条目hit时，将其count设置为0， 其余所有count ++，最后选择一个count数值最大的条目，
		就是最不经常使用的条目，将其反回替换
	2. 先进先出 entry中的count数值记录了entry进入tlb的时间长短，每次替换新的entry进入tlb， 则将其count设置为0
		每次hit都将所有的entry的count++，替换时选择最大的count的entry替换
	3. 随机算法  随机选择一个替换
*/
TranslationEntry *
selectOne(TranslationEntry *list, int size)
{
	int selectMethod = 1;
	switch (selectMethod)
	{
	case 1: // LRU
	case 2: // 先进先出
	{
		int max = 0, pos = 0;
		for (int i = 0; i < size; i++)
		{
			if (!list[i].use)
				return &list[i];
		}
		for (int i = 0; i < size; i++)
		{
			if (list[i].count > max)
			{
				pos = i;
				max = list[i].count;
			}
		}
		list[pos].count = 0;
		return &list[pos];
	}
	case 3:
	{
		return &list[0];
	}
	}
}
/* 
	@author lihaiyang
*/
int Machine::findNullPyhPage()
{
	for (int i = 0; i < NumPhysPages; i++)
	{
		if (pageUsage[i] == false)
		{
			pageUsage[i] = true;
			return i;
		}
	}
	return -1;
}
