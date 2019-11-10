#  Barriers

##### 3017218063 刘兴宇

 ### 题目

barrier障碍：当多个线程不断轮转运行时，每一次轮转，设置一个障碍使得所有的线程在一次轮转后都被阻塞在障碍上。直到所有线程都到达障碍后，在开始下一轮执行

### 解题步骤

- 与pthread中lock有关的函数

  ~~~c
  pthread_mutex_t lock;    	//声明锁
  pthread_mutex_init(&lock, NULL);   // 初始化锁
  pthread_mutex_lock(&lock);  // 加锁
  pthread_mutex_unlock(&lock);  // 释放锁
  ~~~

- 与pthread中全局变量有关的函数

  ~~~C
  pthread_cond_wait(&cond, &mutex);  //睡眠，等待cond锁，释放mutex锁
  pthread_cond_broadcast(&cond); 	//广播，唤醒所有等待cond锁的线程
  ~~~

- 观察程序执行过程

  - 全局变量nthread为障碍阻碍的最大线程数

  - 全局变量round为线程执行轮转数

  - barrier结构体：包括四个属性

    - nthread：当前到达障碍的线程数
    - round：当前的轮转次数
    - barrier_mutex：控制nthread的锁
    - barrie_cond：控制round的锁

  -  在main函数中assert函数调用thread函数，在thread函数中：

    ~~~C
    static void *
    thread(void *xa)
    {
      long n = (long) xa;
      long delay;
      int i;
      
      //轮转20000次
      for (i = 0; i < 20000; i++) {
        int t = bstate.round;
        assert (i == t);//执行对应输出
        barrier();	//进入障碍
        usleep(random() % 100); //冲破障碍后，休眠随机时间，休眠时间最短的线程将执行
      }
    }
    ~~~

  - 只需完成barrier函数

    ~~~C
    static void 
    barrier()
    {
        //为到达线程数变量加锁
    	pthread_mutex_lock(&bstate.barrier_mutex);
    	bstate.nthread++;	//添加到达线程数
    	printf("thread %d in round %d\n", bstate.nthread, bstate.round);
    	if (bstate.nthread != nthread){//如果不是全部线程到达
            //睡眠，等待轮转数的锁，释放到达线程数的锁
    	  	pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
    	}else{ //全部线程到达
      		bstate.round++; //轮转次数+1
    		bstate.nthread = 0; //到达线程数清0
    		pthread_cond_broadcast(&bstate.barrier_cond); //唤醒所有等待轮转数的线程
    	}
    	
        //进入if，执行wait函数在结束睡眠后，会再对到达线程数变量加锁，以保证睡眠前后状态相同，因此需要释放到达线程数变量的锁
        //进入else，结束后仍需释放到达线程数变量的锁
    	pthread_mutex_unlock(&bstate.barrier_mutex);
    }
    ~~~

  - 测试（2线程）

    ~~~shell
    ./a.out 2
    ...
    thread 1 in round 19997
    thread 2 in round 19997
    thread 1 in round 19998
    thread 2 in round 19998
    thread 1 in round 19999
    thread 2 in round 19999
    OK; passed
    ~~~

    更改nthread的值可以进行更大的测试

  