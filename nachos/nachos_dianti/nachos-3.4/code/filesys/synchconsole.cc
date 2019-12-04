#include "copyright.h"
#include "synchconsole.h"

//----------------------------------------------------------------------
// Console RequestDone
// 	Console interrupt handler.  Need this to be a C routine, because
//	C++ can't handle pointers to member functions.
//----------------------------------------------------------------------

static void
ConsoleReadAvail(int arg)
{
    SynchConsole *synchConsole = (SynchConsole *)arg;

    synchConsole->ReadAvailHandler();
}
static void
ConsoleWriteDone(int arg)
{
    SynchConsole *synchConsole = (SynchConsole *)arg;

    synchConsole->WriteDoneHandler();
}

SynchConsole::SynchConsole(char *readFile, char *writeFile)
{
    writeSemaphore = new Semaphore("synch write console", 0);
    readSemaphore = new Semaphore("synch read console", 0);
    writeLock = new Lock("synch console write lock");
    readLock = new Lock("synch console read lock");
    console = new Console(readFile, writeFile, ConsoleReadAvail, ConsoleWriteDone, (int)this);
}

SynchConsole::~SynchConsole()
{
    delete console;
    delete readLock;
    delete writeLock;
    delete readSemaphore;
    delete writeSemaphore;
}

SynchConsole::void PutChar(char ch)
{
    writeLock->Acquire();			// only one disk I/O at a time
    console->PutChar(ch);
    writeSemaphore->P();			// wait for interrupt
    writeLock->Release();
}
char SynchConsole::GetChar()
{
    readLock->Acquire();
    char ch = console->GetChar();
    readSemaphore->P();
    readLock->Release();
    return ch;
}

void SynchConsole::WriteDoneHandler()
{
    writeSemaphore->P();
}
void SynchConsole::ReadAvailHandler()
{
    readSemaphore->P();
}
