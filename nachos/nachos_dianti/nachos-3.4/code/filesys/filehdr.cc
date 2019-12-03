// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

FileHeader::FileHeader(){
	numBytes = 0;
	numSectors = 0;
	for(int i = 0; i < NumIndex; i ++){
		dataSectors[i] = 0;
	}
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool FileHeader::Allocate(BitMap *freeMap, int fileSize)
{
    numBytes = fileSize;
    numSectors = divRoundUp(fileSize, SectorSize);
    if(numSectors < NumDirectIndex){
        if (freeMap->NumClear() < numSectors)
            return FALSE; // not enough space
        for (int i = 0; i < numSectors; i++){
            dataSectors[i] = freeMap->Find();
        }
    }else if(numSectors < Num2Index){
        if (freeMap->NumClear() < numSectors + 1)
            return FALSE; // not enough space
        for (int i = 0; i < NumDirectIndex; i++){
            dataSectors[i] = freeMap->Find();
        }
        dataSectors[NumDirectIndex] = freeMap->Find();
        int* buf = new int[SectorSize/sizeof(int)];
        for(int i = 0; i < numSectors - NumDirectIndex; i ++){
            buf[i] = freeMap->Find();
        }
        synchDisk->WriteSector(dataSectors[NumDirectIndex], (char *)buf);
    }else {
    	return FALSE;
    }
    
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(BitMap *freeMap)
{
    for (int i = 0; i < numSectors; i++)
    {
        ASSERT(freeMap->Test((int)dataSectors[i])); // ought to be marked!
        freeMap->Clear((int)dataSectors[i]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//  如果查找的offset相对应的sector还没有申请，则申请新的sector并返回申请结果，
//  如果超过了一级索引的范围，则需要申请二级索引并进行对应磁盘块的返回
//   传入虚拟文件系统的指针是为了通过文件系统申请新的磁盘块
//----------------------------------------------------------------------

int FileHeader::ByteToSector(int offset, FileSystem* filesys)
{
    int index = offset / SectorSize;
    if(index >= numSectors){    // 长度变化，写操作会发生
        numSectors ++;
        int secNum = filesys->findEmptySector();
        if(secNum == -1){
            ASSERT(FALSE);
        }
        if(index == NumDirectIndex){   //需要新建二级映射
            dataSectors[index] = secNum;
            secNum = filesys->findEmptySector();
            if(secNum == -1)
                ASSERT(FALSE);
            int* buf = new int[SectorSize / sizeof(int)];
            buf[0] = secNum;
            synchDisk->WriteSector(dataSectors[index], (char*)buf);
            return secNum;
        }else if(index > NumDirectIndex){  //已经存在二级映射
            int* buf = new int[SectorSize / sizeof(int)];
            synchDisk->ReadSector(dataSectors[NumDirectIndex], (char*)buf);
            buf[index - NumDirectIndex] = secNum;
            synchDisk->WriteSector(dataSectors[NumDirectIndex], (char*)buf);
            return secNum;
        }else{     // 一级映射
            dataSectors[index] = secNum;
            return secNum;
        }
    }else{        // 长度不变化，不需要新分配磁盘块
        if(index < NumDirectIndex){     // 一级映射
            return dataSectors[index];
        }else{      // 二级映射
            int* buf = new int[SectorSize / sizeof(4)];
            return buf[index - NumDirectIndex];
        }
    }
    
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < numSectors; i++)
        printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < numSectors; i++)
    {
        synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++)
        {
            if ('\040' <= data[j] && data[j] <= '\176') // isprint(data[j])
                printf("%c", data[j]);
            else
                printf("\\%x", (unsigned char)data[j]);
        }
        printf("\n");
    }
    delete[] data;
}

bool FileHeader::initDir(FileSystem* fileSystem) {
	numSectors = 1;
	numBytes = 4;
    int sec = fileSystem->findEmptySector();
    if(sec == -1)
        return FALSE;
    dataSectors[0] = sec;
    char buf[SectorSize] = {};
    synchDisk->WriteSector(sec, buf);
    return TRUE;
}
