# 25 Concurrent Programming



[TOC]



## 并发编程基本概念

* 竞争：程序的正确性依赖于调度的决策

* 死锁：如`printf`不能用在信号处理函数中

  ```c
  #include <stdlib.h>
  #include <stdio.h>
  #include <unistd.h>
  #include <signal.h>
  
  long fork_cnt = 0;
  long handler_cnt = 0;
  
  void sigchld_handler(int sig) {
      while (waitpid(-1, NULL, 0) > 0) {
          printf("Handler reaped a child process, total:%ld\n", ++handler_cnt);
      }
  }
  
  int main() {
      signal(SIGCHLD, sigchld_handler);while (1) {
          if (fork() == 0) {
              exit(0);
          }
          printf("Parent created a child process, total:%ld\n", ++fork_cnt);
      }
      exit(0);
  }
  
  ```

* 活锁：冲突碰撞

* 饥饿：如高优先度进程阻塞低优先度进程





## 基于进程的并发编程

* 服务器接收客户端的连接请求后，服务器`fork`出一个子进程
* 子进程关闭`listenfd`，父进程关闭`connfd`，避免内存泄漏
* 子进程执行完后自动关闭连接，父进程继续监听

优劣：

* 优点：简单，地址空间独立，共享状态信息
* 缺点：难共享信息，进程间通信开销高



## 基于事件的并发编程

* I/O 多路复用的思想：同时监测若干个文件描述符是否可以执行IO操作的能力

  当描述符准备好可读时再去读，读的时候再判断是从哪个描述符读的

* `select`确定要等待的描述符：*读集合*

* 状态机：等待描述符准备好、描述符准备好可以读、从描述符读一个文本行

优劣：

* 优点：一个逻辑控制流，一个地址空间，没有进程/线程管理
* 缺点：编码复杂，只能在一个核上跑





## 基于线程的并发编程

线程：进程上下文中的逻辑流

* 共享代码、数据、堆、共享库和打开的文件
* 私有的线程ID、栈、栈指针、寄存器
* 和一个进程相关的线程组成一个对等线程池
* 上下文切换、创建和终止比进程快

Posix线程：

```c
typedef void *func(void *)
int pthread_create(pthread_t *tid, pthread_attr_t *attr, func *f, void *arg);
pthread_t pthread_self(void);
void pthread_exit(void *thread_return);
int pthread_cancel(pthread_t tid);
int pthread_join(pthread_t tid, void **thread_return);
int pthread_detach(pthread_t tid);
int pthread_once(pthread_once_t *once_control, void (*init_routine)(void));
```

* 运行在同一个进程里的线程共享文件描述符，和关闭逻辑和进程不同
