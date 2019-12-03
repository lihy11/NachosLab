// directory.h
//	Data structures to manage a UNIX-like directory of file names.
//
//      A directory is a table of pairs: <file name, sector #>,
//	giving the name of each file in the directory, and
//	where to find its file header (the data structure describing
//	where to find the file's data blocks) on disk.
//
//      We assume mutual exclusion is provided by the caller.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef DIRECTORY_H
#define DIRECTORY_H

#include "openfile.h"
#include "list.h"
#define FileNameMaxLen 9 // for simplicity, we assume
                         // file names are <= 9 characters long

// The following class defines a "directory entry", representing a file
// in the directory.  Each entry gives the name of the file, and where
// the file's header is to be found on disk.
//
// Internal data structures kept public so that Directory operations can
// access them directly.
/*
 目录项
 1. 标识当前目录项是一个文件还是目录文件
 2. 标识名称，或者长名称存储在一个磁盘块，给出磁盘块号
 3. 标识文件头的磁盘块号。
*/
class DirectoryEntry    
{
public:
	DirectoryEntry(){
		isDirectory = false;
		nameOnDisk = false;
		nameDiskSector = -1;
		createDate = 0;
		lastChangeDate = 0;
		sector = 0;
	};
    bool isDirectory;   // 是否是目录
    bool nameOnDisk;     // 名称是否在磁盘块
    int nameDiskSector;   //名称在磁盘块的位置
    long createDate;    // 标准时间,  创建时间
    long lastChangeDate;   //标准时间， 上次修改

    int sector;                    // Location on disk to find the
                                   //   FileHeader for this file
    char name[FileNameMaxLen + 1] = {}; // Text name for file, with +1 for
                                   // the trailing '\0'
};

// The following class defines a UNIX-like "directory".  Each entry in
// the directory describes a file, and where to find it on disk.
//
// The directory data structure can be stored in memory, or on disk.
// When it is on disk, it is stored as a regular Nachos file.
//
// The constructor initializes a directory structure in memory; the
// FetchFrom/WriteBack operations shuffle the directory information
// from/to disk.
class FileSystem;
class FileHeader;
class Directory
{
public:
    Directory();
    Directory(int sec);
    ~Directory();        // De-allocate the directory

    void FetchFrom(OpenFile *file); // Init directory contents from disk
    void WriteBack(OpenFile *file); // Write modifications to
                                    // directory contents back to disk

    int Find(char *name); // Find the sector number of the
                          // FileHeader for file: "name"

    bool Add(char *name, int newSector, FileSystem* filesys, bool isFile, FileHeader* myHeader); // Add a file name into the directory

    bool Remove(char *name); // Remove a file from the directory

    void List();  // Print the names of all the files
                  //  in the directory
    void Print(); // Verbose print of the contents
                  //  of the directory -- all the file
                  //  names and their contents.

private:
    int tableSize;         // Number of directory entries
    DirectoryEntry* table;           // Table of pairs:
                           // <file name, file header location>

    int FindIndex(char *name); // Find the index into the directory
                               //  table corresponding to "name"
};

#endif // DIRECTORY_H
