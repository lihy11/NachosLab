## Lab3同步机制实验报告
### Exercise 1
##### 中断机制
- 禁用中断：linux内核提供了两个宏：`local_irq_enable, local_irq_disable`，分别用来申请开关中断，使得进程可以实现同步功能
##### 锁机制
- 自旋锁：linux提供了自旋锁，定义在头文件`src/include/linux/spinlock_types.h`中，其结构体如下。
```cpp
typedef struct spinlock {
        union {
                struct raw_spinlock rlock;

#ifdef CONFIG_DEBUG_LOCK_ALLOC
# define LOCK_PADSIZE (offsetof(struct raw_spinlock, dep_map))
                struct {
                        u8 __padding[LOCK_PADSIZE];
                        struct lockdep_map dep_map;
                };
#endif
        };
} spinlock_t;
```
- 读写锁：读写锁的存在，是针对特定的数据读写情况设计的特殊自旋锁，能够并发进行读操作，进而提高程序和系统的性能，其结构体定义在`src/include/linux/rwlock_types.h`中，结构体如下：
```cpp
typedef struct {
        arch_rwlock_t raw_lock;
#ifdef CONFIG_DEBUG_SPINLOCK
        unsigned int magic, owner_cpu;
        void *owner;
#endif
#ifdef CONFIG_DEBUG_LOCK_ALLOC
        struct lockdep_map dep_map;
#endif
} rwlock_t;
```
- 顺序自旋锁,上述的读写锁存在写进程过长等待的情况，因此，顺序自旋锁seqlock给写进程提供更高的优先级，写进程可以抢占读进程，如果读进程被抢占，则此次读取无效，其结构体定义在`src/include/linux/seqlock.h`中，其结构体维护了变量`sequence`，每次写操作都需要更新该变量，而读操作则需要在读取开始和结束检查变量是否变化来确认自己是否曾经被抢占，因此，其官方推荐操作方式如下：
```cpp
* Expected non-blocking reader usage:
*      do {
*          seq = read_seqbegin(&foo);
*      ...
*      } while (read_seqretry(&foo, seq));
```
##### 信号量机制
- 普通信号量：普通信号量除了初始化，只提供`UP，DOWN`两种操作，分别对应释放和申请临界区的控制权，其定义在`src/include/linux/semaphnore.h`中,其结构体如下：
```cpp
struct semaphore {
        raw_spinlock_t          lock;
        unsigned int            count;
        struct list_head        wait_list;
};
```
- 读写信号量，其作用类似于读写锁，适用于读写数据的情况。
- RCU（Read-Copy-Update）操作，也是针对读写操作的一种同步机制，其操作原则应该遵循如下规则
	- 对于读者：调用`rcu_read_lock`和`rcu_read_unlock`函数构建读者临界区，其内部直接获得共享数据的内存指针，能够对其进行读操作，但是操作应该在临界区内部完成
	- 对于写者：首先需要分配新的空间写入数据，然后更新原来的指针为新的指针，达到写的目的