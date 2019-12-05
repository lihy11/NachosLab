// synch.cc
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char *debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff); // disable interrupts

    while (value == 0)
    {                                         // semaphore not available
        queue->Append((void *)currentThread); // so go to sleep
        currentThread->Sleep();
    }
    value--; // semaphore available,
             // consume its value

    (void)interrupt->SetLevel(oldLevel); // re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL) // make thread ready, consuming the V immediately
        scheduler->ReadyToRun(thread);
    value++;
    (void)interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments
// Note -- without a correct implementation of Condition::Wait(),
// the test case in the network assignment won't work!
Lock::Lock(char *debugName)
{
    this->semaphore = new Semaphore(debugName, 1);
    this->heldThread = NULL;
    this->name = debugName;
}
Lock::~Lock()
{
    delete this->semaphore;
}
bool Lock::isHeldByCurrentThread()
{
    return heldThread == currentThread;
}
/*
    获取锁，如果当前锁busy则sleep,获取成功则设置当前的持有线程
*/
void Lock::Acquire()
{
    this->semaphore->P();
    this->heldThread = currentThread;
}
/*
    释放锁，如果当前线程不是持有锁的线程，则直接反回，无权释放，
*/
void Lock::Release()
{
    if (!isHeldByCurrentThread())
    {
        return;
    }
    this->semaphore->V();
    heldThread = NULL;
}

Condition::Condition(char *debugName)
{
    this->name = debugName;
    this->queue = new List;
}
Condition::~Condition()
{
    delete queue;
}
/* 
    等待在条件变量上，操作：加入队列， 释放锁，调度，申请锁
*/
void Condition::Wait(Lock *conditionLock)
{
    this->queue->Append(currentThread);
    conditionLock->Release();
    currentThread->Sleep();
    conditionLock->Acquire();
}
void Condition::Signal(Lock *conditionLock)
{
    if (!this->queue->IsEmpty())
    {
        Thread *t = (Thread *)this->queue->Remove();
        if (t != NULL)
        {
            scheduler->ReadyToRun(t);
        }
    }
}
void Condition::Broadcast(Lock *conditionLock)
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);
    while (!this->queue->IsEmpty())
    {
        Thread *t = (Thread *)this->queue->Remove();
        if (t != NULL)
        {
            scheduler->ReadyToRun(t);
        }
    }
    (void)interrupt->SetLevel(oldLevel);
}

/*
    读者写者问题：基于锁机制
*/
ReadWriteLock::ReadWriteLock()
{
    writeLock = new Lock("write lock");
    readerNumLock = new Lock("reader num lock");
    readerNum = 0;
}
int ReadWriteLock::reader(VoidFunctionPtr func, int file, char *into, int numBytes)
{
    readerNumLock->Acquire();
    readerNum++; //增加读者数量
    if (readerNum == 1)
    { //第一个读者，锁住写操作
        writeLock->Acquire();
    }
    int count =  readerNumLock->Release();

    func(file, into, numBytes); //读取数据

    readerNumLock->Acquire();
    readerNum--;
    if (readerNum == 0)
    { // 这是最后一个读者
        writeLock->Release();
    }
    readerNumLock->Release();
    return count;
}
int ReadWriteLock::writer(VoidFunctionPtr func, int file, char *from, int numBytes)
{
    writeLock->Acquire();
    int count = func(file, from, numBytes); //读取数据
    writeLock->Release();
    return count;
}


/*  读者写者问题，基于条件变量机制  */
class ReadWriteCondition
{
    int AR; //在读者数量
    int WR; //等待读者数量
    int AW; //写者数量
    int WW; //等待读者数量
    Lock *lock;
    Condition *toRead;  //读者等待队列
    Condition *toWrite; //写者等待队列

    void read();
    void write();
    /*
        读者检查是否有读者在读，如果有人在读，则直接读，如果没人读，则检查是否有人写，
        如果有人写，则在读者队列休眠,否则作为第一个读者给写锁
    */
    void reader()
    {
        lock->Acquire();
        while (AW + WW > 0)
        { //有读者在写或者在等
            WR++;
            toRead->Wait(lock); //等待
            WR--;
        }
        AR++; //读者数量增加
        lock->Release();

        read();

        lock->Acquire();
        AR--;
        if (AR == 0 && WW > 0)
        {
            toWrite->Signal(lock);
        }
        lock->Release();
    };
    /*
        如果有人在写或者有人在读，都需要休眠，否则则可以写，写完成后选择唤醒写进程或者读进程
    */
    void writer()
    {
        lock->Acquire();
        while (AR + AW > 0)
        {
            WW++;
            toWrite->Wait(lock);
            WW--;
        }
        AW++;
        lock->Release();

        write();

        lock->Acquire();
        AW--;
        if (WW > 0)
        {
            toWrite->Signal(lock);
        }
        else if (WR > 0)
        {
            toRead->Signal(lock);
        }
        lock->Release();
    };
};
/*  进程同步，多个进程同时到达指定位置可以继续执行  */
class Barrier
{
    Lock *lock;
    int waitingNum; // 已有几个线程到达指定位置
    int runNum;     //需要几个线程到达指定位置
    Condition *toRun;

    void run()
    {
        /*
        ... 执行各个进程相关操作，到达指定位置
        */
        lock->Acquire();
        if (waitingNum < runNum)
        {
            waitingNum++;
            toRun->Wait(lock);
        }
        else
        {
            toRun->Broadcast(lock);
        }
        /*
        ... 执行后续操作
        */
    }
};
