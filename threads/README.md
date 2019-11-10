# Threads and Locking

##### 3017218063 刘兴宇

### 前提

- 使用哈希表来测试多线程。每一个线程运行分为两个阶段：

  - put阶段：向哈希表插入数据（更新全部数据）

  - get阶段：读取哈希表中的全部数据

- 程序通过计算时间来比较多线程程序与单线程程序
  - put time：代表线程put阶段消耗的时间
  - get time：代表线程get阶段消耗的时间
  - completion time：代表程序从开始到所有线程都执行完成消耗的时间
- key missing：代表在线程读取数据时丢失的数据数（因为线程间数据抢夺而导致多个线程同时对一个值进行修改）
  - 单线程key missing永远为0

### 题目

- 编译：

  ~~~shell
  gcc -g -O2 ph.c -pthread
  ~~~

- 首先，要先将虚拟机CPU内核设置为2核及以上，同时不能同时运行大量占用CPU的应用程序

- 双线程运行：

  ~~~shell
  ./a.out 2
  ~~~

  得到结果为：

  ~~~linux
  1: put time = 0.010260
  0: put time = 0.010362
  0: get time = 8.624186
  0: 17704 keys missing
  1: get time = 8.621099
  1: 17704 keys missing
  completion time = 8.637058
  ~~~

- 单线程运行

  ~~~shell
  ./a.out 1
  ~~~

  得到结果为

  ~~~
  0: put time = 0.003926
  0: get time = 8.837044
  0: 0 keys missing
  completion time = 8.841244
  ~~~

- 观察可以看出双线程运行并不比单线程快，并且还会丢失大量数据

- 要求：修改代码中的`get`和`put`函数，为之加锁，确保数据不会丢失

### 解题步骤

- 与pthread中lock有关的函数

  ~~~C
  pthread_mutex_t lock;    	//声明锁
  pthread_mutex_init(&lock, NULL);   // 初始化锁
  pthread_mutex_lock(&lock);  // 加锁
  pthread_mutex_unlock(&lock);  // 释放锁
  ~~~

- 打开`ph.c`

  - 在全局变量声明锁

    ~~~c
    pthread_mutex_t lock; 
    ~~~

  - 在main函数头部初始化锁
  
    ~~~C
    pthread_mutex_init(&lock, NULL); 
    ~~~
  
  - 修改put函数：
  
    ~~~c
    static 
    void put(int key, int value)
    {
      pthread_mutex_lock(&lock);
      int i = key % NBUCKET;
      insert(key, value, &table[i], table[i]);
      pthread_mutex_unlock(&lock);
    }
    ~~~
  
    put函数加锁，同时只能有一个线程执行此函数
  
  - get函数无需修改，因为get操作不会使数据丢失
  
- 测试

  - 多线程

    ~~~
    0: put time = 0.003853
    1: put time = 0.003955
    0: get time = 11.178099
    0: 0 keys missing
    1: get time = 11.188428
    1: 0 keys missing
    completion time = 11.200665
    ~~~

  - 单线程

    ~~~
    0: put time = 0.011124
    0: get time = 10.089338
    0: 0 keys missing
    completion time = 10.100779
    ~~~

  - 观察到因为加锁，导致运行变慢，但数据不丢失