// threadtest.cc
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield,
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void SimpleThread(int which)
{
    int num;

    for (num = 0; num < 5; num++)
    {
        printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, (void *)1);
    SimpleThread(0);
}

/*
    @lihaiyang 线程测试函数，打印用户id和线程id
*/
void SimpleThread2(int which)
{
    if (which < 10)
    {
        for (int i = 0; i < 3; i++)
        {
            printf("我的名字是 : %s, 我的用户是 : %d, 我的tid是： %d\n",
                   currentThread->getName(), currentThread->getUserID(), currentThread->getTid());
            currentThread->Yield();
        }
    }
    else
    {
        for (int i = 0; i < 30; i++)
        {
            printf("我的名字是 : %s, 我的用户是 : %d, 我的tid是： %d\n",
                   currentThread->getName(), currentThread->getUserID(), currentThread->getTid());
            currentThread->Yield();
        }
    }
}
/*
    @lihaiyang 线程测试，循环创建线程直到线程池溢出
*/
void ThreadTest2()
{
    DEBUG('t', "Entering Thread Test 2");
    Thread *t = NULL;
    while ((t = new Thread("Test Thread")) != NULL)
    {
        if (t->getTid() == -1)
        {
            break;
        }
        t->Fork(SimpleThread2, (void *)1);
    }
    SimpleThread2(0);
}
/*
    测试优先级线程调度算法
*/

void ThreadTest3()
{
    Thread *t3 = new Thread("thread 3", 0, 3);
    t3->Fork(SimpleThread2, (void *)1);
    for (int i = 0; i > 30; i++)
        ;

    Thread *t2 = new Thread("thread 2", 0, 2);
    t2->Fork(SimpleThread2, (void *)1);
    for (int i = 0; i > 30; i++)
        ;

    Thread *t1 = new Thread("thread 1", 0, 1);
    t1->Fork(SimpleThread2, (void *)1);
    for (int i = 0; i > 30; i++)
        ;
}

void SimpleThread4(int which)
{
    for (int i = 0; i < 300; i++)
    {
    //    printf("我的名字是 : %s, 我的tick是： %d\n",
    //           currentThread->getName(), currentThread->getTicks());
    }
}
/*  时间片轮转算法 */
void ThreadTest4()
{
    Thread *t1 = new Thread("thread 1");
    Thread *t2 = new Thread("thread 2");
    t1->Fork(SimpleThread4, (void *)1);
    t2->Fork(SimpleThread4, (void *)1);
    SimpleThread4(0);
}
//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void ThreadTest()
{
    switch (testnum)
    {
    case 1:
        ThreadTest1();
        break;
    case 2:
        ThreadTest2();
        break;
    case 3:
        ThreadTest3();
        break;
    case 4:
        ThreadTest4();
        break;
    default:
        printf("No test specified.\n");
        break;
    }
}
