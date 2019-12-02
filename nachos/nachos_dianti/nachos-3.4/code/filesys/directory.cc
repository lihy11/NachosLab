// directory.cc
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"
#include <time.h>
//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

/*
@author lihaiyang
无参数构造函数，真正的无限制文件夹的构造函数
*/
Directory::Directory()
{
    tableSize = 0;
    table = NULL;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{
    if (table != NULL)
        delete[] table;
}

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//   存储格式如下：tablesize , Dentry , Dentry ...，先读取前4个字节的tablesize， 再依次读取
//
//----------------------------------------------------------------------

void Directory::FetchFrom(OpenFile *file)
{
    (void)file->ReadAt(&tableSize, 4, 0); //读取entry数量

    int fileSize = tableSize * sizeof(DirectoryEntry); // 获取entry 总字节数
    (void)file->ReadAt((char *)table, fileSize, 4);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void Directory::WriteBack(OpenFile *file)
{
    (void)file->WriteAt(&tableSize, 4, 0); //写入entry数量

    (void)file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 4);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (!strncmp(table[i].name, name, FileNameMaxLen))
            return i;
    return -1; // name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
        return table[i].sector;
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool Directory::Add(char *name, int newSector, FileSystem* filesys, bool isFile)
{
    if (FindIndex(name) != -1)
        return FALSE;

    /*  分配新的比原来大一个entry 的 table， 删除原来的*/
    tableSize++;
    DirectoryEntry *newTable = new DirectoryEntry[tableSize];
    strncpy(newTable, table, sizeof(DirectoryEntry) * (tableSize - 1));
    delete table;
    table = newTable;
    /*  拷贝名字*/
    int nameLen = strlen(name);
    if (nameLen > FileNameMaxLen)
    { //  名字写入磁盘块 nameSector
        int nameSector = filesys->findEmptySector();
        table[tableSize - 1].nameDiskSector = nameSector;
    }
    else
    {
        strncpy(table[i].name, name, FileNameMaxLen);
    }
    time_t create;
    time(&create);
    table[tableSize - 1].createDate = (long)create;
    table[tableSize - 1].sector = newSector;
    if(isFile){
        table[tableSize-1].isDirectory = FALSE;
    }else{
        table[tableSize-1].isDirectory = true;
    }
    return TRUE;
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory.
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool Directory::Remove(char *name)
{
    int i = FindIndex(name);

    if (i == -1)
        return FALSE; // name not in directory
    tableSize --;
    DirectoryEntry* buf = new DirectoryEntry[tableSize];
    int firstPartSize = i * sizeof(DirectoryEntry);
    int secondPartSize = (tableSize - i) * sizeof(DirectoryEntry);
    strncpy(buf, table, firstPartSize);
    strncpy(buf + firstPartSize, table + firstPartSize + sizeof(DirectoryEntry), secondPartSize);
    delete table;
    table = buf;
    return TRUE;
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory.
//----------------------------------------------------------------------

void Directory::List()
{
    for (int i = 0; i < tableSize; i++){
        if(!table[i].nameOnDisk){
            printf("%s, %l\n", table[i].name, table[i].createDate);
        }else{
            printf("too long name to show, %l\n", table[i].createDate);
        }
    }
            
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void Directory::Print()
{
    FileHeader *hdr = new FileHeader;
    Directory* dir = new Directory();
    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++){
        printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
        hdr->FetchFrom(table[i].sector);
        hdr->Print();
    }
    printf("\n");
    delete hdr;
}
