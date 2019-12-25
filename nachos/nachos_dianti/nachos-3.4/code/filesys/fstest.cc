// fstest.cc
//	Simple test routines for the file system.
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"
#include "directory.h"
#define TransferSize 64 // make it small, just to be difficult

//----------------------------------------------------------------------
// Copy
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void Copy(char *from, char *to) {
	FILE *fp;
	OpenFile *openFile;
	int amountRead, fileLength;
	char *buffer;

	// Open UNIX file
	if ((fp = fopen(from, "r")) == NULL) {
		printf("Copy: couldn't open input file %s\n", from);
		return;
	}

	// Figure out length of UNIX file
	fseek(fp, 0, 2);
	fileLength = ftell(fp);
	fseek(fp, 0, 0);

	// Create a Nachos file of the same length
	DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
	if (!fileSystem->Create(to, fileLength, TRUE)) { // Create Nachos file
		printf("Copy: couldn't create output file %s\n", to);
		fclose(fp);
		return;
	}

	openFile = fileSystem->Open(to);
	ASSERT(openFile != NULL);

	// Copy the data in TransferSize chunks
	buffer = new char[TransferSize];
	while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
		openFile->Write(buffer, amountRead);
	delete[] buffer;

	// Close the UNIX and the Nachos files

	fclose(fp);
}

//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void Print(char *name) {
	OpenFile *openFile;
	int i, amountRead;
	char *buffer;

	if ((openFile = fileSystem->Open(name)) == NULL) {
		printf("Print: unable to open file %s\n", name);
		return;
	}

	buffer = new char[TransferSize];
	while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
		for (i = 0; i < amountRead; i++)
			printf("%c", buffer[i]);
	delete[] buffer;

	delete openFile; // close the Nachos file
	return;
}

//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName "/home/li/test"
#define Contents "1234567890"
#define ContentSize strlen(Contents)
#define FileSize ((int)(ContentSize * 5000))

static void FileWrite() {
	OpenFile *openFile;
	int i, numBytes;

	printf("Sequential write of %d byte file, in %d byte chunks\n",
	FileSize, ContentSize);
	if (!fileSystem->Create(FileName, 0, TRUE)) {
		printf("Perf test: can't create %s\n", FileName);
		return;
	}
	openFile = fileSystem->Open(FileName);
	if (openFile == NULL) {
		printf("Perf test: unable to open %s\n", FileName);
		return;
	}
	for (i = 0; i < FileSize; i += ContentSize)
	{
		numBytes = openFile->Write(Contents, ContentSize);
		if (numBytes < 10) {
			printf("Perf test: unable to write %s\n", FileName);
			delete openFile;
			return;
		}
	}
	delete openFile; // close file
}

static void FileRead() {
	OpenFile *openFile;
	char *buffer = new char[ContentSize];
	int i, numBytes;

	printf("Sequential read of %d byte file, in %d byte chunks\n",
	FileSize, ContentSize);

	if ((openFile = fileSystem->Open(FileName)) == NULL) {
		printf("Perf test: unable to open file %s\n", FileName);
		delete[] buffer;
		return;
	}
	for (i = 0; i < FileSize; i += ContentSize)
	{
		numBytes = openFile->Read(buffer, ContentSize);
		if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize)) {
			printf("Perf test: unable to read %s\n", FileName);
			delete openFile;
			delete[] buffer;
			return;
		}
	}
	delete[] buffer;
	delete openFile; // close file
}

void PerformanceTest() {
	printf("Starting file system performance test:\n");
	stats->Print();
	FileWrite();
	FileRead();
	if (!fileSystem->Remove(FileName)) {
		printf("Perf test: unable to remove %s\n", FileName);
		return;
	}
	stats->Print();
}
void testSynchRead() {
	OpenFile* file = fileSystem->Open("/home/li/test");
	for (int i = 0; i < 5; i++) {
		char buf[10] = { };
		fileSystem->fread(file, buf, 1);
		printf("thread %s read 1 char from file , get char : %s\n",
				currentThread->getName(), buf);
		currentThread->Yield();
	}
}
void testSynchWrite() {
	OpenFile* file = fileSystem->Open("/home/li/test");
	for (int i = 0; i < 5; i++) {
		char* buf = "abcdefghijklmn";
		fileSystem->fwrite(file, buf, 1);
		printf("thread %s write 1 char from file , get char : %c\n",
				currentThread->getName(), buf[i]);
		currentThread->Yield();
	}
}

void pipeRead(int arg) {
	PipeFile* pipe = (PipeFile*) arg;
	char buf[10] = { };

	pipe->read(buf, 1);
	printf("i an thread %s, you input %c in last thread\n",
			currentThread->getName(), buf[0]);

}
void pipeWrite(int arg) {
	PipeFile* pipe = (PipeFile*) arg;
	printf(" i am thread %s , please input 1, 2, or 3\n",
			currentThread->getName());
	char c[10];
	scanf("%s", &c);
	pipe->write(c, 1);
}
void testFileSystem() {
   fileSystem->Create("/home", 0, FALSE);
   fileSystem->Create("/tmp", 0, FALSE);
   fileSystem->Create("/home/li", 0, FALSE);
   char* name1 = "/home/lihaiyang/NachosLab/nachos/nachos_dianti/nachos-3.4/code/test/thread1";
   char* name2 = "/home/lihaiyang/NachosLab/nachos/nachos_dianti/nachos-3.4/code/test/thread2";
   char* name3 = "/home/lihaiyang/NachosLab/nachos/nachos_dianti/nachos-3.4/code/test/sort";
   Copy(name1, "/home/li/thread1");
   Copy(name2, "/home/li/thread2");
   Copy(name3, "/home/li/sort");
//   Thread* t1 = new Thread("thread1");
//   t1->Fork(testSynchRead, 0);
////   Thread* t2 = new Thread("thread2");
////   t2->Fork(testSynchRead, 0);
//   Thread* t3 = new Thread("thrad3");
//   t3->Fork(testSynchWrite, 0);

	/*
//	 * pipe test
//	 * */
//	PipeFile* pipe = new PipeFile();
//	Thread* write = new Thread("write");
//	write->Fork(pipeWrite, (void*) pipe);
//	Thread* read = new Thread("read");
//	read->Fork(pipeRead, (void*) pipe);
}
