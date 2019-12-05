# XV6 Lazy Page Allocation

##### 3017218063 刘兴宇

### 题目

在XV6系统中，对内存的分配采用的是按需分配的策略。当一个进程需要更多的内存时，用户调用`malloc()`函数，通过调用`sbrk()`系统调用分配内存。但是进程在分配给内存后，有可能根本用不到这些内存，会导致大量的内存被浪费

题目要求改造XV6系统，使得它实现懒分配机制。即许诺给进程请求的内存空间，但是在内存不使用时不实际分配这些空间。当进程实际使用时，产生缺页中断，再为进程分配空间。

### 步骤

- 修改`sbrk()`系统调用，使得进程请求时，只增加大小，并不实际分配内存

  打开`sysproc.c`，修改`sys_sbrk()`函数：

  ~~~C
  int
  sys_sbrk(void)
  {
    int addr;
    int n;
  
    if(argint(0, &n) < 0)
      return -1;
    addr = myproc()->sz;
    myproc()->sz += n; //增加进程已申请的内存空间
    //if(growproc(n) < 0) //删除分配内存的代码
    //  return -1;
    return addr;	//返回原地址末尾
  }
  ~~~

  此时打开xv6，运行`echo hi`，会产生如下缺页中断

  ![image-20191205225750579](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20191205225750579.png)

- 添加懒分配代码

  - `vm.c`中`mappages`函数用于将已分配的内存映射到虚拟地址中，为了在懒分配代码中使用此函数，需要去除它的static属性，并且在`trap.c`中声明：

    ~~~C
    extern int mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm);
    ~~~

  - 在`trap`函数中`default`选项中cprinf前，对懒分配产生的缺页中断进行处理，添加如下代码：

    ~~~C
    char* mem;
    uint a;
    a = PGROUNDDOWN(rcr2()); //a用于获得产生缺页中断的虚拟地址
    uint newsz = myproc()->sz; //进程认为自己获得的内存最大地址
    for ( ; a < newsz ; a += PGSIZE){ //对于已许诺但未分配空间	
    	mem = kalloc();	//分配空间
    	memset(mem, 0, PGSIZE); 
    	mappages(myproc()->pgdir, (char*)a, PGSIZE, V2P(mem), PTE_W|PTE_U);		//将新分配空间与产生缺页中断逻辑地址映射
    }
    break;
    ~~~

    进程对内存空间使用时连续的，第一次使用许诺但未分配的空间时会产生缺页中断，此时缺页中断的逻辑地址是未分配空间的最低地址。进程认为自己已经获得的内存最大地址减去产生缺页中断的地址就是已许诺但未分配的地址空间

  - 重新测试

    ![image-20191205225825815](C:\Users\Administrator\AppData\Roaming\Typora\typora-user-images\image-20191205225825815.png)

    测试成功