# XV6-System Call

##### 3017218063 刘兴宇



## Part1

### 题目

修改`syscall.c`，在操作系统内核进行系统调用时，打印系统调用编号、名称、以及系统调用参数（32位系统调用中使用寄存器存储）

### 步骤

首先阅读`syscall.h`文件：

~~~C
#define SYS_fork    1
#define SYS_exit    2
#define SYS_wait    3
#define SYS_pipe    4
...
#define SYS_unlink 18
#define SYS_link   19
#define SYS_mkdir  20
#define SYS_close  21
~~~

发现系统调用编号以及它们对应的名称，再观察`syscall.c`文件：

~~~C
//定义的外部函数：系统调用对应的函数
extern int sys_chdir(void);
extern int sys_close(void);
extern int sys_dup(void);
...
extern int sys_write(void);
extern int sys_uptime(void);

//系统调用编号作为数组索引，系统调用函数（地址）作为值，将系统调用编号与函数相关联
static int (*syscalls[])(void) = {
[SYS_fork]    sys_fork,
[SYS_exit]    sys_exit,
[SYS_wait]    sys_wait,
...
[SYS_mkdir]   sys_mkdir,
[SYS_close]   sys_close,
};
~~~

再观察`syscall()`函数

~~~C
void
syscall(void)
{
  int num;
  struct proc *curproc = myproc(); //proc为系统当前的一些参数，包括寄存器值、CPU状态等
  num = curproc->tf->eax; //获得当前eax寄存器的值，即系统调用编号
  
  //判断编号是否存在
  if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    curproc->tf->eax = syscalls[num](); //将系统调用函数地址存入eax寄存器，便于后面的执行程序读取
  } else {
    cprintf("%d %s: unknown sys call %d\n",
            curproc->pid, curproc->name, num);
    curproc->tf->eax = -1; //对于不存在的系统调用，传递错误值-1
  }
}
~~~

可以发现，在`if`语句判断为`true`后，对应的系统调用将被执行。为了输出系统调用的名称，在`syscall()`函数中建立如下C字符串数组`systemCallName`，同样使用系统调用编号作为索引，系统调用名称作为值。

~~~C
char* syscallNames[22] = {
    [SYS_fork]    "fork",
    [SYS_exit]    "exit",
    [SYS_wait]    "wait",
    [SYS_pipe]    "pipe",
    [SYS_read]    "read",
    [SYS_kill]    "kill",
    [SYS_exec]    "exec",
    [SYS_fstat]   "fstat",
    [SYS_chdir]   "chdir",
    [SYS_dup]     "dup",
    [SYS_getpid]  "getpid",
    [SYS_sbrk]    "sbrk",
    [SYS_sleep]   "sleep",
    [SYS_uptime]  "uptime",
    [SYS_open]    "open",
    [SYS_write]   "write",
    [SYS_mknod]   "mknod",
    [SYS_unlink]  "unlink",
    [SYS_link]    "link",
    [SYS_mkdir]   "mkdir",
    [SYS_close]   "close"
  };
~~~

又观察`proc.h`中有关`proc` 和其中的`context`属性结构：

~~~C
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};
~~~

得知寄存器的存储方式。

添加如下语句，按照“系统调用名 -> 编号“格式打印，并且在Register寄存器后逐行打印各寄存器中的参数值。

~~~C
if(num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    curproc->tf->eax = syscalls[num](); 

    //新增代码段
    cprintf("\n %s -> %d\n Registers:\n", syscallNames[num-1], num);
    cprintf(" edi: %d\n esi: %d\n ebx: %d\n ebp: %d\n eip: %d\n", curproc->context[0], curproc->context[1], curproc->context[2], curproc->context[3], curproc->context[4]);
    //新增代码段结束
}
~~~

保存更改，打开`qemu`，观察运行结果（篇幅有限，中间部分省略）

~~~shell
 kill -> 7
 Registers:
 edi: -2146420988
 esi: 134
 ebx: 0
 ebp: -1912603784
 eip: -2146420988

 uptime -> 15
 Registers:
 edi: -2146420988
 esi: -1912603144
 ebx: -1912603012
 ebp: -1912603268
 eip: -2146420988
.
.
.
$
 open -> 16
 Registers:
 edi: -2146358176
 esi: 0
 ebx: -1912607368
 ebp: -2146420988
 eip: 130
 
 open -> 16
 Registers:
 edi: -2146358176
 esi: 0
 ebx: -1912607368
 ebp: -2146420988
 eip: 130

~~~

由于输出到shell上的操作也为系统调用，因此每次shell输出字符都会输出系统调用信息。

## Part2

### 题目

增加一个系统调用date，通过该系统调用可以在shell上显示当前的时间

### 步骤

- 获取系统当前时间的方法已经在`lapic.c`中的`comstime()`函数中实现，该函数不接受任何参数，返回一个`trcdate`结构体实例。

- 在`Ubuntu`控制台中输入如下命令：

  ~~~shell
  grep -n uptime *.[chS]
  ~~~

  可以查看到uptime系统调用在XV6源码中出现的位置，供后续步骤参考：

  ~~~linux
  syscall.c:105:extern int sys_uptime(void);
  syscall.c:122:[SYS_uptime]  sys_uptime,
  syscall.c:152:    [SYS_uptime]  "uptime",
  syscall.h:15:#define SYS_uptime 14
  sysproc.c:95:sys_uptime(void)
  user.h:25:int uptime(void);
  usys.S:32:SYSCALL(uptime)
  ~~~

- 在`syscall.h`中增加系统调用编号：22

  ~~~C
  #define SYS_date   22
  ~~~

- 在`syscall.c`中定义外部系统调用函数、在系统调用名数组和编号与函数映射数组中添加对应的内容

  ~~~C
  //添加外部系统调用函数
  extern int sys_date(void);
  
  //添加系统调用名称和系统调用函数的映射关系
  static int (*syscalls[])(void) = {
  	...
      [SYS_date]    sys_date,
  }
  
  //添加系统调用名
  char* syscallNames[23] = {
          [SYS_date]    "date",
  };
  ~~~

- 在`sysproc.c`中实现在`syscall.c`中定义的外部函数`sys_date()`

  ~~~C
  int
  sys_date(struct rtcdate *r){
   
    //argptr用于判断参数是否合法（空指针等），接受三个参数：
    //要判断的参数在参数列表中索引号，参数的地址，参数的大小
    //返回值!=0代表参数有问题
    if (argptr(0, (void*)&r, sizeof(r)) < 0){ 
      return 1;
    }else{
      cmostime(r); //获取当前时间，存入结构体r中
      return 0; //正常返回
    }
  
  }
  ~~~

- 在`user.h`中定义用户函数`date`，是用户使用系统调用的接口

  ~~~C
  //system calls
  ...
  int date(struct rtcdate*);
  ~~~

- 在`usys.S`中定义转换规则，其中的汇编代码可以将用户函数转换为系统调用函数

  ~~~C
  SYSCALL(uptime)
  ~~~

- 新建文件`date.c`，作为测试程序

  ~~~C
  #include "types.h"
  #include "user.h"
  #include "date.h"
  
  int
  main(int argc, char *argv[])
  {
    struct rtcdate r;
  
    if (date(&r)) { //date()进行系统调用，返回0代表成功，否则代表失败
      printf(2, "date failed\n");
      exit();
    }
  
    // your code to print the time in any format you like...
    //转换为GMT+8
    r.hour += 8;
    if (r.hour > 24){
      r.hour -= 24; 
    }
    
    //输出
    printf(1, "Now time(GMT+8): %d-%d-%d %d:%d:%d\n", r.year, r.month, r.day, r.hour, r.minute, r.second);
  
    exit();
  }
  ~~~

- 打开`qemu`，执行指令

  ~~~shell
  $ date
  ~~~

  得到如下结果（为了便于查看结果，注释了part1中的语句）

  ~~~linux
  Now time(GMT+8): 2019-10-12 21:56:56
  ~~~

  