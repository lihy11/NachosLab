// scheduler.cc
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would
//	end up calling FindNextToRun(), and that would put us in an
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize the list of ready but not running threads to empty.
//----------------------------------------------------------------------

Scheduler::Scheduler()
{
    this->scheduleMethod = RR; //TODO 可调整
    this->RunTicks = 100;
    readyList = new List;
    readyList2 = new List;
    readyList3 = new List;
    this->threadPool.clear();
    for (int i = 0; i < MAX_THREAD_NUMBER; i++)
    {
        this->tids.push(i);
        threadListLeval[i] = 0;
    }
}

//----------------------------------------------------------------------
// Scheduler::~Scheduler
// 	De-allocate the list of ready threads.
//----------------------------------------------------------------------

Scheduler::~Scheduler()
{
    delete readyList;
    delete readyList2;
    delete readyList3;
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void Scheduler::ReadyToRun(Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());

    thread->setStatus(READY);
    if (this->scheduleMethod == MULTIQUEUE)
    { //多级反馈队列
        if (checkRunTime(thread))
        {
            switch (this->threadListLeval[thread->getTid()])
            {
            case 0:
                readyList2->Append(thread);
                threadListLeval[thread->getTid()] = 2;
                break;
            case 2:
            case 3:
                threadListLeval[thread->getTid()] = 3;
                readyList3->Append(thread);
                break;
            }
        }
        else
        {
            switch (this->threadListLeval[thread->getTid()])
            {
            case 0:
                readyList->Append(thread);
                threadListLeval[thread->getTid()] = 0;
                break;
            case 2:
                readyList->Append(thread);
                threadListLeval[thread->getTid()] = 2;
                break;
            case 3:
                threadListLeval[thread->getTid()] = 3;
                readyList2->Append(thread);
                break;
            }
        }
    }
    else
    {
        readyList->Append((void *)thread);
    }
}

//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun()
{
    switch (this->scheduleMethod)
    {
    case PRIORITY:
        return priority();
    case RR:
        return runtimeRound();
    case MULTIQUEUE:
        return multiPriorityQueue();
    }
}

//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void Scheduler::Run(Thread *nextThread)
{
    Thread *oldThread = currentThread;

#ifdef USER_PROGRAM // ignore until running user programs
    if (currentThread->space != NULL)
    {                                   // if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
        currentThread->space->SaveState();
    }
#endif

    oldThread->CheckOverflow(); // check if the old thread
                                // had an undetected stack overflow

    currentThread = nextThread;        // switch to the next thread
    currentThread->setStatus(RUNNING); // nextThread is now running

    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
          oldThread->getName(), nextThread->getName());

    // This is a machine-dependent assembly language routine defined
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    SWITCH(oldThread, nextThread);

    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL)
    {
        delete threadToBeDestroyed;
        threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL)
    {                                      // if there is an address space
        currentThread->RestoreUserState(); // to restore, do it.
        currentThread->space->RestoreState();
    }
#endif
}

//----------------------------------------------------------------------
// Scheduler::Print
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------
void Scheduler::Print()
{
    printf("Ready list contents:\n");
    readyList->Mapcar((VoidFunctionPtr)ThreadPrint);
}

/**/
int Scheduler::aquireTid(Thread *t)
{

    if (tids.size() == 0)
    {
        return -1;
    }
    int tid = tids.front();
    tids.pop();
    threadPool.push_back(t);
    return tid;
}

/**/
void Scheduler::releaseTid(Thread *t)
{
    //  DEBUG('t', "DEBUG:  Thread addr : %x\n", t);
    if (t->getTid() == -1)
    {
        return;
    }
    //  DEBUG('t', "DEBUG:  tid : %d\n", t->getTid());
    tids.push(t->getTid());
    for (std::vector<Thread *>::iterator t0 = threadPool.begin(); t0 < threadPool.end(); t0++)
    {
        //    printf("DEBUG  : Thread pools : %x\n", *t0);
        if (*t0 == t)
        {
            threadPool.erase(t0);
            break;
        }
    }
}
/*
    检查是否存在优先级更高的进程等待
*/
bool Scheduler::checkPriority(Thread *curT)
{
    if (this->scheduleMethod == MULTIQUEUE)
    {
        int l = threadListLeval[curT->getTid()];
        switch (l)
        {
        case 0:
            return false;
        case 2:
            return !readyList->IsEmpty();
        case 3:
            return (!readyList->IsEmpty()) && (!readyList2->IsEmpty());
        }
    }
    else
    {
        if (this->scheduleMethod != PRIORITY)
        {
            return false;
        }
        ListElement *first = readyList->getHead();
        ListElement *ptr;

        for (ptr = first; ptr != NULL; ptr = ptr->next)
        {
            if (curT->getPriority() > ((Thread *)ptr->item)->getPriority())
            {
                return true;
            }
        }
    }
}
/*
    检查是否运行完成时间片
*/
bool Scheduler::checkRunTime(Thread *curT)
{
    if (scheduleMethod == MULTIQUEUE)
    {
        switch (threadListLeval[curT->getTid()])
        {
        case 0:
            return curT->getTicks() > this->RunTicks;
        case 2:
            return curT->getTicks() > 2 * this->RunTicks;
        case 3:
            return curT->getTicks() > 4 * this->RunTicks;
        }
    }
    else
    {
        return curT->getTicks() > this->RunTicks;
    }
}
/*
    基于优先级的抢占算法，当调度时依次遍历队列，选取其中优先级数值最小的进程。
*/
Thread *
Scheduler::priority()
{

    ListElement *first = readyList->getHead();
    ListElement *ptr;
    Thread *result = NULL;
    int pri = 3;
    for (ptr = first; ptr != NULL; ptr = ptr->next)
    {
        Thread *tmp = (Thread *)ptr->item;
        if (tmp->getPriority() < pri)
        {
            pri = tmp->getPriority();
            result = tmp;
        }
    }
    if (result == NULL)
    {
        return (Thread *)readyList->Remove();
    }
    else
    {
        readyList->Remove(result);
        return result;
    }
}

/*
    时间片轮转算法，当当前进程时间片消耗完则发生调度
*/
Thread *
Scheduler::runtimeRound()
{
    return (Thread *)readyList->Remove();
}
/* 多级反馈队列*/
Thread *
Scheduler::multiPriorityQueue()
{
    if(!readyList->IsEmpty()){
        return (Thread *)readyList->Remove();
    }else if(!readyList2->IsEmpty()){
        return (Thread *)readyList2->Remove();
    }else{
    return (Thread *)readyList3->Remove();
    }
}

Thread* Scheduler::getThreadByTid(int tid){
    for (std::vector<Thread *>::iterator t0 = threadPool.begin(); t0 < threadPool.end(); t0++)
    {
        if (*t0->getTid() == tid)
        {
            return *t0;
        }
    }
    return NULL;
}