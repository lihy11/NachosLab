// scheduler.h
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#define MAX_THREAD_NUMBER 128

#include "copyright.h"
#include "list.h"
#include "thread.h"
#include <vector>
#include <queue>
// The following class defines the scheduler/dispatcher abstraction --
// the data structures and operations needed to keep track of which
// thread is running, and which threads are ready but not running.

enum ThreadSchedulingMethod{
    PRIORITY,
    RR,
    MULTIQUEUE
};
class Scheduler
{
public:
    Scheduler();  // Initialize list of ready threads
    ~Scheduler(); // De-allocate ready list

    void ReadyToRun(Thread *thread); // Thread can be dispatched.
    Thread *FindNextToRun();         // Dequeue first thread on the ready
                                     // list, if any, and return thread.
    void Run(Thread *nextThread);    // Cause nextThread to start running
    void Print();                    // Print contents of ready list

private:
    List *readyList; // queue of threads that are ready to run,
                     // but not running


private:
    /*  @lihaiyang 维护可用tid，维护线程池  */
    std::vector<Thread *> threadPool;
    std::queue<int> tids;
    ThreadSchedulingMethod scheduleMethod;
    static int RunTicks;
public:
    /* @lihaiyang  申请一个tid，申请不到则反回-1 ,否则反回tid，并将线程指针加入线程池  */
    int aquireTid(Thread *t);
    void releaseTid(Thread *t);
    
    /* @lihaiyang 检查是否有优先级更高的进程在等待*/
    bool checkPriority(Thread *curT);
    bool checkRunTime(Thread* curT);

    /*线程调度方式*/
    Thread* priority();  //抢占式优先级调度
    Thread* runtimeRound();    //时间片轮转
    Thread* multiPriorityQueue();   //多级反馈队列

};

#endif // SCHEDULER_H
