# XV6 bigger files

##### 3017218063 刘兴宇

### 题目

XV6操作系统文件管理系统中，Inode的data区使用一级间接索引指针存储文件数据。共13个指针，前12个为直接索引，直接指向一个数据块，第13个指针为一级简介索引，指向一个全部为指针的数据块，每个指针指向一个数据块。因此XV6文件的最大大小为70KB

现在要将该文件系统改为可以支持二级间接索引，要将前11个指针设为直接索引，第12个指针作为一级间接索引指针，第13个指针作为二级间接索引指针。

在下载了`big.c`到XV6中后，使用`big`命令，可以得到当前可创建的最大文件所占的块数：

~~~shell
$ big
...
wrote 140 sectors
done; ok
~~~

当前为140个

### 步骤

- 准备，修改文件系统中的代码：

  - `makefile`：

  	~~~C++
  	CPUS := 1;	//修改CPUS的值为1
  	QEMUEXTRA = -snapshot; //在QEMUOPTS前添加这一行
		~~~
  
  - `mkfs`：
  
    ~~~C
    #define FSSIZE 20000 //扩大文件系统的大小 
    ~~~
  
- 观察并理解`fs.h`中dinode（存储在磁盘上的Inode）结构

  ~~~C
  struct dinode {
    short type;           // 文件类型
    short major;          // Major device number (T_DEV only)
    short minor;          // Minor device number (T_DEV only)
    short nlink;          // 文件链接数
    uint size;            // 文件大小
    uint addrs[NDIRECT+1];   // 文件数据（指针数组）
  };
  ~~~

- 修改`fs.h`中的常量

  ~~~C
  #define NDIRECT 11 //直接指针的数目减小为11个
  #define NINDIRECT (BSIZE / sizeof(uint)) //一级间接索引的最大指针数，不修改
  #define NININDIRECT NINDIRECT * NINDIRECT //新增，二级间接索引的最大指针数
  #define MAXFILE (NDIRECT + NINDIRECT + NININDIRECT) //改变文件最大大小
  ~~~

- 修改相应的Inode定义

  `fs.h`中的`struct dinode`中`addrs`数组初始大小变为：

  ~~~C++
  uint addrs[NDIRECT+2];
  ~~~

  同理`file.h`中的`struct inode`中`addrs`数组初始大小变为：

  ~~~C++
  uint addrs[NDIRECT+2];
  ~~~

- 阅读并理解`fs.c`中的`bmap()`函数

  该函数可以理解为将inode中的指针映射到真实的物理块中

  ~~~C++
  //输入文件ip和要读取的数据指针编号bn，输出对应数据块的物理地址addr
  bmap(struct inode *ip, uint bn)
  {
    uint addr, *a;
    struct buf *bp; //bp是一个缓冲区
  
    if(bn < NDIRECT){ //当bn属于直接指针时
       /*如果该指针没有指向任何数据块，就为它分配一个新的空数据块
        *并将该块地址addr返回
        *否则返回ip的addr[bn]的值，即物理地址
        */
      if((addr = ip->addrs[bn]) == 0)
        ip->addrs[bn] = addr = balloc(ip->dev);
      return addr;
    }
    bn -= NDIRECT; //减去直接指针个数，得到在一级间接指针中所求指针的序号
  
    if(bn < NINDIRECT){ //bn为间接指针
      // 如果间接指针块没有创建，就为它分配一个新的空格数据块
      if((addr = ip->addrs[NDIRECT]) == 0)
        ip->addrs[NDIRECT] = addr = balloc(ip->dev);
      bp = bread(ip->dev, addr); //将存放一级间接指针的数据块加载到缓冲区
      a = (uint*)bp->data; //获取一级间接指针的数据块中的间接指针数组
       
      /*如果对应的间接指针未指向任何数据块，则分配一个新的空数据块
       *并返回其地址
       *否则返回对应数据块物理地址
       */
      if((addr = a[bn]) == 0){ 
        a[bn] = addr = balloc(ip->dev);
        log_write(bp); //如果进行了修改，就要回写间接指针数据块
      }
      brelse(bp);//释放缓冲区
      return addr;
    }
  
    panic("bmap: out of range");
  }
  
  ~~~

- 修改`bmap()`函数，在` panic("bmap: out of range")`前增加：

  ~~~C++
    bn -= NINDIRECT; //再减去一级间接指针的个数，此时bn为二级间接指针中的序号
  
    if (bn < NININDIRECT){ //序号小于二级间接指针的最大个数（前提条件）
    	uint indexOfNindirect = 0; //用于存储bn在第几个二级间接指针块内
      while (bn >= NINDIRECT){ //循环，每次减少一个二级间接指针块的最大指针数，直到bn不足一块
  		bn -= NINDIRECT;
  		indexOfNindirect++;
  	}
  	
  	if ((addr = ip->addrs[NDIRECT+1]) == 0){ //不存在存放二级间接指针的一级间接指针块
  		ip->addrs[NDIRECT+1] = addr = balloc(ip->dev); //分配空间
  	}
  	bp = bread(ip->dev, addr); //读取存放二级间接指针的一级间接指针块
  	a = (uint*)bp->data;
  	
  	if ((addr = a[indexOfNindirect]) == 0){ //不存在二级间接指针块，分配并回写
  		a[indexOfNindirect]	= addr = balloc(ip->dev);
  		log_write(bp);
  	}
  	bq = bread(ip->dev, addr); //读取二级间接指针块
  	b = (uint*)bq->data;
  
  	if ((addr = b[bn]) == 0){ //对应数据块不存在，分配并回写
  		b[bn] = addr = balloc(ip->dev);	
  		log_write(bq);
  	}
  	brelse(bq);	//释放二级指针块
  	brelse(bp);	//释放一级指针块
  	return addr;
    }
  
  ~~~

- 再在XV6上运行`big`命令，得到如下结果

  ~~~shell
  $
  ...............................................................................................................................................................................
  wrote 16533 sectors
  done; ok
  ~~~

  二级索引建立成功

