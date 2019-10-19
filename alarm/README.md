# XV6 CPU alarm

##### 3017218063 刘兴宇

### 题目

添加一个系统调用`alarm`，功能为在程序使用了一定数量的CPU时间之后会发出警告，实现用户级的中断/异常处理程序。

如果应用程序调用`alarm(n, fn)`，则程序会在每消耗n个CPU时间之后，调用异常处理程序fn

### 步骤

- 添加系统调用`alarm`
  - 在`syscall.h`中增加系统调用号

  	~~~C
  	#define SYS_alarm  23
  	~~~

  - 在`syscall.c`中增加系统调用的外部函数声明和系统调用函数与系统调用编号的映射关系

  	~~~C
  	extern int sys_alarm(void);
  	
  	static int (*syscalls[])(void) = {
  	...
  	[SYS_alarm]   sys_alarm,
  	};
  	~~~
  
  - 在`user.h`中增加系统调用的用户函数声明
  
    ~~~C
    int alarm(int ticks, void (*handler)());
    ~~~
  
  - 在`usys.S`中定义从用户函数到系统调用函数的转换规则
  
    ~~~C
    SYSCALL(alarm)
    ~~~
  
- 为存储程序运行上下文的`proc`结构体添加`alarm`所需要的参数

  在`proc.h`中的`struct proc`结构体增加属性：

  ~~~C
  struct proc {
    ...
  
    int alarmticks;				//alarm发出警告需要的时间数
    void (* alarmhandler)();		//触发警告后的异常处理函数（函数指针）
    int count;					//当前已经使用时间数
  };
  ~~~

- 在`proc.c`中增加`proc`结构体重新增的属性的初始化操作

  ~~~C
  static struct proc*
  allocproc(void)
  {
    ...
    p->count = 0;					//当前已使用值初始化为0
    return p;   
  }
  ~~~

- 在`sysproc.c`中实现系统调用函数

  ~~~C
  int
  sys_alarm(void)
  {
    int ticks;
    void (*handler)();
  
    if(argint(0, &ticks) < 0)
      return -1;
    if(argptr(1, (char**)&handler, 1) < 0)
      return -1;
    myproc()->alarmticks = ticks;		//用户传入的ticks值（间隔值）赋予上下文的alarmticks参数
    myproc()->alarmhandler = handler; //用户传入的异常处理函数赋予上下文的alarmhandler参数
    return 0;
  }
  ~~~

- 在`trap.c`中，改变系统中断中`case T_IRQ0 + IRQ_TIMER`分支的代码，实现系统调用被执行时`alarm`的判断逻辑

  ~~~C
  void
  trap(struct trapframe *tf)
  {
    ...
    case T_IRQ0 + IRQ_TIMER:
  	...
     	if(myproc() != 0 && (tf->cs & 3) == 3){
        (myproc()->count)++;		//增加目前的CPU时间计数
        if (myproc()->count == myproc()->alarmticks){ //到达时间上限
          myproc()->count = 0;	//计数重置为0
          tf->esp -= 4;			//扩充栈空间
          *((uint*) (tf->esp)) = tf->eip;	//将此时下一条指令地址压栈
          tf->eip = (uint)(myproc()->alarmhandler); //将异常处理函数的地址赋予eip程序计数器，使之执行
        }
      }
      lapiceoi();
      break;
    ...
  }
  ~~~

- 编写测试程序`alarmtest.c`

  ~~~C
  #include "types.h"
  #include "stat.h"
  #include "user.h"
  
  void periodic();
  
  int
  main(int argc, char *argv[])
  {
    int i;
    printf(1, "alarmtest starting\n");
    alarm(10, periodic);	//每10个时间执行periodic处理函数
    for(i = 0; i < 250*500000; i++){
      if((i % 250000) == 0)
        write(2, ".", 1);	//迭代执行write系统调用
    }
    exit();
  }
  
  void
  periodic() 
  {
    printf(1, "alarm!\n"); //alarm处理函数：输出“alarm！”
  }
  ~~~

  并将该程序添加至`makefile`

- 编译`qemu`，减慢程序运行速度，使结果更明显

  ~~~shell
  make CPUS=1 qemu
  ~~~

- 在`qemu`上运行命令

  ~~~shell
  $ alarmtest
  ~~~

  得到如下结果

  ~~~shell
  $ alarmtest
  alarmtest starting
  ..........................alarm!
  ..................alarm!
  ...................alarm!
  .......................................alarm!
  ...........................................................................................................alarm!
  ................................................................................................................................alarm!
  ...........................................................................................................alarm!
  ~~~

  

  