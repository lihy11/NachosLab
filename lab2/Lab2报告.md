## Lab2 线程调度 实验报告
#### Exercise1 
- 对于linux中的普通进程来说，调度算法采取完全公平调度算法(CFS)，其根本思想是让每一个进程能够分配的到相同时间的CPU使用权，如果某个进程为IO密集型进程，其CPU使用时间较短，则调度时则会考虑让其有更高的优先级上CPU，以弥补其在CPU使用时间上的劣势。
- linux对于每个进程维护一个`nice`值和一个`vruntime`值，`nice`值代表进程的优先级，`nice`取值`-20`到`+19`，值越小代表进程的优先级越高。但是`nice`值确不会直接影响进程调度，是通过影响`vruntime`来影响进程调度，`vruntime`是进程在CPU的虚拟使用时间，其计算方式为`实际运行时间×NICE_0_LOAD/权重`，每个进程的`vruntime`会累加，在调度时选取`vruntime`值最小的进程上CPU。而`NICE_0_LOAD`代表nice值为0的权重，`权重`变量则依照不同的nice值对应至不同的至，进而按照程序的优先级对其占用CPU的时间进行对应的缩放变成`vruntime`，产生不同的优先级调度的效果。
- 对于实时进程，linux采用了两种调度方式，SCHED_FIFO和SCHED_RR，即简单的先进先出和时间片轮转算法。但是每个进程的优先级都高于普通进程。
- Windows 实现了一个优先驱动的，抢先式的调度系统——具有最高优先级的可运行线程总是运行，而该线程可能仅限于在允许它运行的处理器上运行，这种现象称为处理器亲和性，在默认的情况下，线程可以在任何一个空闲的处理器上运行，但是，你可以使用windows 调度函数，或者在映像头部设置一个亲和性掩码来改变处理器亲和性。也就是在一定程度上将进程和CPU绑定
- 参考链接：https://blog.csdn.net/deyili/article/details/6420034；https://blog.csdn.net/lenomirei/article/details/79274073；https://www.cnblogs.com/lenomirei/p/5516872.html