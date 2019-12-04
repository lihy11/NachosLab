#include "copyright.h"

#ifndef SYNCHDISK_H
#define SYNCHDISK_H

#include "console.h"
#include "synch.h"


class SynchConsole {
  public:
    SynchConsole(char *readFile, char *writeFile);
				// initialize the hardware console device
    ~SynchConsole();			// clean up console emulation

// external interface -- Nachos kernel code can call these
    void PutChar(char ch);	// Write "ch" to the console display, 
				// and return immediately.  "writeHandler" 
				// is called when the I/O completes. 

    char GetChar();	   	// Poll the console input.  If a char is 
				// available, return it.  Otherwise, return EOF.
    				// "readHandler" is called whenever there is 
				// a char to be gotten

// internal emulation routines -- DO NOT call these. 
    void WriteDoneHandler();	 	// internal routines to signal I/O completion
    void ReadAvailHandler();

  private:
    Console *console;		  		// Raw disk device
    Semaphore *writeSemaphore; 		// To synchronize requesting thread 
					// with the interrupt handler
    Semaphore* readSemaphore;
    Lock *writeLock;		  		// Only one read/write request
					// can be sent to the disk at a time
    Lock* readLock;
};

#endif // SYNCHDISK_H
