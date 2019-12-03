// filesys.cc
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known
// sectors, so that they can be located on boot-up.
#define FreeMapSector 0
#define RootDirectorySector 1
#define FreeMapDataSector 2
// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number
// of files that can be loaded onto the disk.
#define FreeMapFileSize (NumSectors / BitsInByte)
#define NumDirEntries 10
#define DirectoryFileSize (sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------

FileSystem::FileSystem(bool format) {
	DEBUG('f', "Initializing the file system.\n");
	if (format) {
		DEBUG('f', "Formatting the file system.\n");
		BitMap *freeMap = new BitMap(NumSectors);
		FileHeader *mapHdr = new FileHeader;

		/*  标记空闲数据块的文件头的磁盘块号 */
		freeMap->Mark(FreeMapSector);
		freeMap->Mark(RootDirectorySector);   // 标记根目录文件头数据块

		/*  给空闲位图分配文件数据块*/
		ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));

		/*  将空闲位图head写回磁盘 */
		mapHdr->WriteBack(FreeMapSector);

		/*  将空闲位图以文件形式打开  */
		freeMapFile = new OpenFile(FreeMapSector);
		freeMapFile->filesys = this;

		freeMap->WriteBack(freeMapFile);
		/*  创建根目录 */
		Directory *root = new Directory();
		FileHeader *rootDirHdr = new FileHeader;
		ASSERT(rootDirHdr->initDir(this));

		/*  将根目录文件头写回指定位置 */
		rootDirHdr->WriteBack(RootDirectorySector);

		/*  将根目录以文件形式打开 */
		rootDirectoryFile = new OpenFile(RootDirectorySector);
		rootDirectoryFile->filesys = this;

		if (DebugIsEnabled('f')) {
			freeMap->Print();
			root->Print();

			delete freeMap;
			delete root;
			delete mapHdr;
			delete rootDirHdr;
		}
	} else {
		// if we are not formatting the disk, just open the files representing
		// the bitmap and directory; these are left open while Nachos is running
		freeMapFile = new OpenFile(FreeMapSector);
		rootDirectoryFile = new OpenFile(RootDirectorySector);
	}
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//        Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   		file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created
//----------------------------------------------------------------------

bool FileSystem::Create(char *name, int initialSize, bool isFile) {
    int sector;
    bool success;

    DEBUG('f', "Creating dir %s\n", name);

    int fatherSec = findFatherDirectory(name, RootDirectorySector);
    OpenFile* fatherFile = new OpenFile(fatherSec);
    Directory* fatherDir = new Directory();
    fatherDir->FetchFrom(fatherFile);
    FileHeader* fatherHdr = new FileHeader();
    fatherHdr->FetchFrom(fatherSec);

    if (fatherDir->Find(name) != -1) {
        success = FALSE; // file is already in directory
    } else {
        sector = findEmptySector();
        if (sector == -1)
            success = FALSE; // no free block for file header
        else {
            fatherDir->Add(getFileName(name), sector, this, isFile, fatherHdr);
            FileHeader* hdr = new FileHeader;
            if(isFile == FALSE)
                hdr->initDir(this);
            hdr->WriteBack(sector);
            fatherHdr->WriteBack(fatherSec);
            fatherDir->WriteBack(fatherFile);
            success = TRUE;
            delete hdr;
        }
    }
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.
//	To open a file:
//	  Find the location of the file's header, using the directory
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------

OpenFile *
FileSystem::Open(char *name) {

	Directory *directory = new Directory(findFatherDirectory(name, RootDirectorySector));
	OpenFile *openFile = NULL;
	int sector;
	DEBUG('f', "Opening file %s\n", name);
	sector = directory->Find(name);
	if (sector >= 0)
		openFile = new OpenFile(sector); // name was found in directory
	delete directory;
	openFile->filesys = this;
	return openFile; // return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------

bool FileSystem::Remove(char *name) {
	Directory *directory;
	BitMap *freeMap;
	FileHeader *fileHdr;
	int sector;

	directory = new Directory(findFatherDirectory(name, RootDirectorySector));
	sector = directory->Find(name);
	if (sector == -1) {
		delete directory;
		return FALSE; // file not found
	}
	fileHdr = new FileHeader;
	fileHdr->FetchFrom(sector);

	freeMap = new BitMap(NumSectors);
	freeMap->FetchFrom(freeMapFile);

	fileHdr->Deallocate(freeMap); // remove data blocks
	freeMap->Clear(sector);       // remove header block
	directory->Remove(name);

	freeMap->WriteBack(freeMapFile);     // flush to disk
	directory->WriteBack(rootDirectoryFile); // flush to disk
	delete fileHdr;
	delete directory;
	delete freeMap;
	return TRUE;
}

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------

void FileSystem::List() {
	Directory *directory = new Directory();

	directory->FetchFrom(rootDirectoryFile);
	directory->List();
	delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------

void FileSystem::Print() {
	FileHeader *bitHdr = new FileHeader;
	FileHeader *dirHdr = new FileHeader;
	BitMap *freeMap = new BitMap(NumSectors);
	Directory *directory = new Directory();

	printf("Bit map file header:\n");
	bitHdr->FetchFrom(FreeMapSector);
	bitHdr->Print();

	printf("Directory file header:\n");
	dirHdr->FetchFrom(RootDirectorySector);
	dirHdr->Print();

	freeMap->FetchFrom(freeMapFile);
	freeMap->Print();

	directory->FetchFrom(rootDirectoryFile);
	directory->Print();

	delete bitHdr;
	delete dirHdr;
	delete freeMap;
	delete directory;
}

/*
 查找分配空闲磁盘块
 */
int FileSystem::findEmptySector() {
	BitMap* freeMap = new BitMap(NumSectors);
	freeMap->FetchFrom(freeMapFile);
	int sec = freeMap->Find();
	freeMap->WriteBack(freeMapFile);
	return sec;
}

/*
 分割文件名称， 获得其所属文件夹
 */

int FileSystem::findFatherDirectory(char* name, int pwdSec) {
	char* tmp = name;
	if (*tmp == '/') {
		tmp++;
		name++;
	}
	int len = 0;
	while (*tmp != '\0' && tmp != NULL && *tmp != '/') {
		len++;
		tmp++;
	}
	char* fileName = new char[len+1];
	memcpy(fileName, name, len);
	fileName[len] = '\0';
	if (tmp == NULL || *tmp == '\0') {  // 文件
        return pwdSec;
	} else {       // 文件夹
	    Directory* pwd = new Directory(pwdSec);
        int childSec = pwd->Find(fileName);
        if (childSec == -1)
            return -1;
		return findFatherDirectory(tmp, childSec);
	}
}

char* FileSystem::getFileName(char* abName){
	int lastSep = -1;
	int i = 0;
	char* tmp = abName;
	for(int j = strlen(abName)-1; j >= 0; j --){
		if(abName[j] == '/'){
			lastSep = j;
			break;
		}
	}
	if(lastSep == -1)
		return abName;
	return abName + lastSep + 1;
}

