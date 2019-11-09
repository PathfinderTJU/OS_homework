# SysV Semaphore

##### 3017218063 刘兴宇

### 题目

使用SysV Semaphore实现经典IPC问题——读写问题（读者优先）

### 运行方法

- 进入当前目录下：`cd my_semaphore`
- 编译：`make`
- 初始化：`./init`，将全部（20）数据初始化值为其下标
- 运行读者程序：
   - `./reader`：按序读取全部数据，每读取一个数据休眠1秒。输出格式为：`Data+index：value`
   - `./reader index`：读取下标为`index`（合法范围0-19，否则报错）的数据。输出格式为：`Data+index：value`
- 运行写者程序：
   - `./writer`：修改全部数据的值为`4*index`，每修改一个数据休眠1秒，并且输出修改后的数据值，输出格式为：`Data+index：value`
   - `./writer index value`：修改下标为`index`（合法范围0-19，否则报错）的数据值为`value`（必须为整数值，未做错误检测，输入字符串可能会导致逻辑错误），并且输出修改后的数据值，输出格式为：`Data+index：value`

- IPC效果
  - 运行读者程序时
    - 如果当前有其它读者程序正在运行，则新的读者程序可以运行
    - 如果当前有写者程序性正在运行，新的读者程序将等待
  - 运行写者程序时
    - 无论当前有读者还是写者程序在运行，新的写者程序都将等待
- 注意：不要强行停止程序，会导致信号量的操作出现问题。如果不得不强行停止，请重新使用`./init`初始化 

### 解题步骤

- 观察示例代码结构：

  - 在`myipc.h`中定义了IPC所需要的一些方法和变量，需要关注的有：

    ~~~C
    //定义信号量、共享内存区的唯一标识符“key”
    #define KEY_MUTEX 100//信号量mutex
    #define KEY_EMPTY 101//信号量empty
    #define KEY_FULL 102//信号量full
    #define KEY_SHM	200//共享内存区
    #define BUFFER_SIZE 10//共享内存区中数组的大小
    
    //用于信号量操作传递参数的union
    union semun {
    	int val;
    	struct semid_ds *buf;
    	unsigned short int *array;
    	/*struct seminfo *__buf;*/
    };
    
    //共享内存区的结构体
    struct shared_use_st {
    	int buffer[BUFFER_SIZE];//存放商品的数组
    	//int count;
    	int lo;//商品的最小下标
    	int hi;//商品的最大下标
    	int cur;//指针
    };
    
    //PV原语
    extern int sem_p(semaphore sem_id);
    extern int sem_v(semaphore sem_id);
    ~~~

  - 在`myipc.c`中实现了IPC所需要的函数，需要关注的有：

    ~~~C
    //semop用于改变信号量的值，参数为:
    //sem_id：信号量的key
    //sem_b：参数结构体，其中sem_op为1时代表P原语，-1时代表V原语，其它参数均固定
    //返回值-1代表改变失败
    
    //P原语
    int sem_p(semaphore sem_id)
    {
        struct sembuf sem_b;
        sem_b.sem_num = 0;
        sem_b.sem_op = -1; /* P() */
        sem_b.sem_flg = SEM_UNDO;
        if (semop(sem_id, &sem_b, 1) == -1) {
            fprintf(stderr, "semaphore_p failed\n");
            return(0);
        	}
        return(1);
    }
    
    //V原语
    int sem_v(semaphore sem_id)
    {
        struct sembuf sem_b;
        sem_b.sem_num = 0;
        sem_b.sem_op = 1; /* V() */
        sem_b.sem_flg = SEM_UNDO;
        if (semop(sem_id, &sem_b, 1) == -1) {
            fprintf(stderr, "semaphore_v failed\n");
            return(0);
        	 }
       return(1);
    }
    ~~~

  - `init`中对变量、共享内存进行了初始化

    ~~~C
    #include <unistd.h>
    #include <stdlib.h>
    #include <stdio.h>
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/sem.h>
    #include <sys/shm.h>
    #include <sys/msg.h>
    #include "myipc.h"
    
    int main(int argc, char *argv[])
    {
        int i,producer_pid,consumer_pid,item,shmid;
    	semaphore mutex,empty,full; //定义信号量
        union semun sem_union;	//定义操作信号量的参数union
    	void *shared_memory = (void *)0;   //定义指向贡献内存的指针
    	struct shared_use_st *shared_stuff;//定义共享内存存储的结构图
    
       /*
       	*获取信号量KEY，赋予对应变量
        *semget函数用于创建一个新信号量或取得一个已有信号量
        *第一个参数为信号量的KEY，如果信号量未建立，则新建
        *其它参数均为固定值
        *返回值对应信号量的KEY
        */
    	if ( (mutex=semget((key_t)KEY_MUTEX,1,0666|IPC_CREAT)) == -1 ) {
    		fprintf(stderr,"Failed to create semaphore!"); 
    		exit(EXIT_FAILURE);
    	}
    	if ( (empty = semget((key_t)KEY_EMPTY,1,0666|IPC_CREAT)) == -1 ) {
    		fprintf(stderr,"Failed to create semaphore!"); 
    		exit(EXIT_FAILURE);
    	}
    	if ( (full = semget((key_t)KEY_FULL,1,0666|IPC_CREAT)) == -1 ) {
    		fprintf(stderr,"Failed to create semaphore!"); 
    		exit(EXIT_FAILURE);
    	}
        
       /*
       	*获取共享内存KEY，赋予shmid
        *shmid用于获得或分配共享内存
        *第一个参数为共享内存的KEY，如果共享内存不存在，则新建一个
        *第二个参数为共享内存的大小，此处为结构图大小
        *其余参数均固定
        *返回值为对应共享内存的KEY
        */
        
    	if ( (shmid = shmget((key_t)KEY_SHM,sizeof(struct shared_use_st),0666|IPC_CREAT)) == -1 ) {
    		fprintf(stderr,"Failed to create shared memory!"); 
    		exit(EXIT_FAILURE);
    	}
    
       /*
       	*初始化信号量的值
        *semctl用于控制信号量
        *第一个参数为要控制的信号量的KEY
        *第二个参数固定为0
        *第三个参数为SETVAL代表初始化操作
        *第四个参数为参数union，其中的val属性为初始化的值
        */
        
        	sem_union.val = 1;
        	if (semctl(mutex, 0, SETVAL, sem_union) == -1) {
    		fprintf(stderr,"Failed to set semaphore!"); 
    		exit(EXIT_FAILURE);
    	}
    
        	sem_union.val = 0;
        	if (semctl(full, 0, SETVAL, sem_union) == -1) {
    		fprintf(stderr,"Failed to set semaphore!"); 
    		exit(EXIT_FAILURE);
    	}
    
        	sem_union.val = BUFFER_SIZE;
        	if (semctl(empty, 0, SETVAL, sem_union) == -1) {
    		fprintf(stderr,"Failed to set semaphore!"); 
    		exit(EXIT_FAILURE);
    	}
    	
       /*
        *获取对共享内存的使用
       	*shmat函数用于将刚创建的共享内存启动访问
       	*返回值为指向共享内存的第一个指针
        */
        
    	if ( (shared_memory = shmat(shmid,(void *)0,0) ) == (void *)-1) {
    		fprintf(stderr,"shmat failed\n");
    		exit(EXIT_FAILURE);
    	}
        
        //将共享内存绑定到shared_stuff上
    	shared_stuff = (struct shared_use_st *)shared_memory;
    
        //初始化商品队列及指针
    	for(i=0;i<BUFFER_SIZE;i++)
    	{
    		shared_stuff->buffer[i] = 0;
    	}
    	shared_stuff -> lo = 0;
    	shared_stuff -> hi = 0;
    	shared_stuff -> cur = 0;
    
      	exit(EXIT_SUCCESS);
    }
    ~~~

  - `producer.c`中定义了生产者的函数

    ~~~C
    for(i=0;i<30;i++)//最多生产30个
    	{
    		//item为下一个要生产的index
    		item = ++(shared_stuff->cur);
    		sleep(1);
    		printf("Producing item %d\n",item);
        	//P
    		sem_p(empty);
    		sem_p(mutex);
    		//更新生产队列，并修改指针值
    		(shared_stuff->buffer)[(shared_stuff->hi)]=item;
    		(shared_stuff->hi) = ((shared_stuff->hi)+1) % BUFFER_SIZE;
    		printf("Inserting item %d\n",item);
    		//V
    		sem_v(mutex);
    		sem_v(full);
    	}
    
    	//释放共享内存
        if (shmdt(shared_memory) == -1) {
           	fprintf(stderr, "shmdt failed\n"); 
    		exit(EXIT_FAILURE);
    	}
    
    	printf("Finish!\n");
    
    	getchar();//
    ~~~

  - `consumer.c`类似定义了消费者函数，不在赘述

- 实现读写

  - `myipc.h`中：

    ~~~c
    //定义两个信号量：mutex控制数据存储区，rdcntmumtex控制读者数
    #define KEY_MUTEX 100
    #define KEY_RDCNTMUTEX 101
    #define KEY_SHM	200
    #define BUFFER_SIZE 20
    
    //定义共享内存结构体，包括一个大小为20个int的存储区域和一个表示读者程序数目的reader_count
    struct shared_use_st {
    	int reader_count;
    	int buffer[BUFFER_SIZE];
    };
    ~~~

  - `myipc.c`不做任何修改

  - `init.c`中，除去对响应信号量的获取和初始化操作外，初始化共享内存：

    ~~~C
    //数据区初始化为下标值
    for(i=0;i<BUFFER_SIZE;i++)
    {
    	shared_stuff->buffer[i] = i;
    }
    
    //reader_count初始化为0
    shared_stuff->reader_count = 0;
    ~~~

  - `reader.c`中实现读者函数

    ~~~C
    //判断参数类型是否合法，参数长度只可能为1或2
    int main(int argc, char* argv[]){
    	int type;
    	if (argc == 2){
    		type = 1;	
    	}else if (argc == 1){
    		type = 2;	
    	}else{
    		fprintf(stderr,"Invalid Arguments\n"); 
    		exit(EXIT_FAILURE);
    	}
        
    	...
        
        //增加reader_count，当是第一个执行的读者程序时，占用数据区资源，同时让后面进入的读者程序无需再申请占用数据区资源
    	sem_p(rdcntmutex);
    	if (shared_stuff->reader_count == 0)
    	{
    		sem_p(mutex);
    	}
    	shared_stuff->reader_count = shared_stuff->reader_count + 1;
    	sem_v(rdcntmutex);
    
        //输出特定的数据
    	if (type == 1){
    		i = atoi(argv[1]);//获取要输出的数据下标（注意类型转换）
    		if (i < 0 || i > 19){//判断下标是否合法
    			fprintf(stderr,"out of range\n");
    			exit(EXIT_FAILURE);
    		}
    		item = shared_stuff->buffer[i];
    		sleep(1);
    		printf("Data %d: %d\n",i, item);
    	}else if (type == 2){
    		for(i=0;i<BUFFER_SIZE;i++)//输出全部数据
    		{
    			item = shared_stuff->buffer[i];
    			sleep(1);
    			printf("Data %d: %d\n",i, item);
    
    		}
    	}else{
    		fprintf(stderr,"invalid arguments\n");
    		exit(EXIT_FAILURE);
    	}
    	
        //减少reader_count，若自己是最后一个退出的读者程序，释放数据空间
    	sem_p(rdcntmutex);
    	shared_stuff->reader_count = shared_stuff->reader_count - 1;
    	if (shared_stuff->reader_count == 0)
    	{
    		sem_v(mutex);
    	}
    	sem_v(rdcntmutex);
        
        ...
          
    }
    ~~~

  - `writer.c`中实现了写者函数

    ~~~c
    int main(int argc, char *argv[])
    {
    	//判断参数合法性，参数长度只可能为1或3
    	int type;
    	if (argc == 3){
    		type = 1;	
    	}else if (argc == 1){
    		type = 2;	
    	}else{
    		fprintf(stderr,"Invalid Arguments\n"); 
    		exit(EXIT_FAILURE);
    	}
    	
    	...
    	
    	//占用数据区
    	sem_p(mutex);
    	if (type == 1){//修改某个数据
    		item = atoi(argv[2]);//获取下标
    		i = atoi(argv[1]);//获取值
    		if(i < 0 || i > 19){//判断下标合法性
    			fprintf(stderr,"out of range\n");
    			exit(EXIT_FAILURE);
    		}
    		shared_stuff->buffer[i] = item;
    		printf("Set %d: %d\n", i, item);
    		sleep(1);
    	}else if (type == 2){//修改全部数据
    		for(i=0;i<BUFFER_SIZE;i++)
    		{
    			item = i*3+i;
    			shared_stuff->buffer[i] = item;
    			printf("Set %d: %d\n", i, item);
    			sleep(1);
    		}
    	}else{
    		fprintf(stderr,"Invalid Arguments"); 
    		exit(EXIT_FAILURE);	
    	}
    	sem_v(mutex);//释放数据区
    	
    	...
    	
    }
    ~~~

    

