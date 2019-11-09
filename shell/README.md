# 6.828 Shell

##### 3017281063 刘兴宇

### 题目要求

完善`6.828shell`的代码，实现一个linux的shell程序，能够读取命令、解析并执行。

- 支持的命令：

  - 可执行命令：如cat、ls、rm
- 重定向命令：>、<
  - 管道：|

只需编写具体的执行过程

### 解题思路

- linux文件指针：

  linux每个进程都维护一个文件指针表fd：其中0代表STDIN、1代表STDOUT、2代表STDERR

- 阅读`sh.c`代码，观察shell的执行过程

  - 命令分类：shell将命令分为三类

    - 可执行命令exec（' '）：可以直接执行的文件
    - 重定向命令redir（<或>）：带有输入、输出重定向符的命令
    - 管道命令pipes（|）：带有管道符的命令

  - 数据结构

    - `struct cmd`：命令通用结构体，包括一个属性：命令类型
    - `struct execcmd`：可执行命令结构体，包括属性：命令类型、参数数组
    - `struct redircmd`：重定向命令结构体，包括属性：命令类型、要运行的命令结构体、输入/输出文件名、标识读取/写入文件的标志符flag、
    - `struct pipecmd`：管道命令结构体，包括属性：命令类型、管道左侧命令结构体、管道右侧命令结构体

  - 命令执行过程：main函数

    ~~~C
    int main(void)
    {
      static char buf[100];
      int fd, r;
    
      // getcmd从键盘读入命令，存储在缓冲区char数组buf
      while(getcmd(buf, sizeof(buf)) >= 0){
        if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
          //当命令是cd时
          buf[strlen(buf)-1] = 0;  // 过滤掉\n
          if(chdir(buf+3) < 0)
            fprintf(stderr, "cannot cd %s\n", buf+3);
          continue;
        }
        //创建子进程，执行命令
        //其中，parsecmd（buf）命令对缓冲区的命令进行处理，转换为结构体，填充参数等操作
        //runcmd运行处理后的命令
        if(fork1() == 0)
          runcmd(parsecmd(buf));
        wait(&r);
      }
      exit(0);
    }
    ~~~

  - 可以观察到，执行命令的函数位于`runcmd`中的`switch`语句中，只需要填写三个类别的`case`语句中的代码

  - 可执行命令

    ~~~C
    case ' ':
        ecmd = (struct execcmd*)cmd;
        if(ecmd->argv[0] == 0)
          _exit(0);
        
    	//access函数是验证第一个参数对应的文件（命令本质上也是文件），第二个参数F_OK是对文件存在性进行验证。存在则返回0
    	//首先使用相对路径判断命令是否存在，存在则执行
    	if (access(ecmd->argv[0], F_OK) == 0){ 
            //execv会挂起当前进程，并用第一个参数的应用进程替换当前进程，并传入对应参数。
    		execv(ecmd->argv[0], ecmd->argv);
    	}else{
            //未在相对变量中找到，就在绝对变量中寻找
            //两个绝对变量的根目录：/bin/（内置）与/usr/bin/（用户内置）
    		char* binpath[2] = {"/bin/", "/usr/bin/"};
            
            //为两种绝对路径分配空间
    		char* tempStr = (char*)malloc((strlen(ecmd->argv[0]) + strlen(binpath[0]))*sizeof(char));
    		char* tempStr2 = (char*)malloc((strlen(ecmd->argv[0]) + strlen(binpath[1]))*sizeof(char));
            
            //构造绝对路径
            //strcpy为复制第二个参数的字符串到第一个参数的字符串
    		strcpy(tempStr, binpath[0]);
    		strcpy(tempStr2, binpath[1]);
            
            //strcat为将第二个参数字符串添加到第一个参数字符串末尾
    		strcat(tempStr, ecmd->argv[0]);
    		strcat(tempStr2, ecmd->argv[0]);
       
       		//使用/bin/绝对路径查找命令     
    		if (access(tempStr, F_OK) == 0){
    			execv(tempStr, ecmd->argv);
    		}else{
                //使用/usr/bin/绝对路径录查找命令
    			if (access(tempStr2, F_OK) == 0){
    				execv(tempStr2, ecmd->argv);
    			}
    			else{//命令未找到，报错
    				fprintf(stderr, "%s: Command not found\n", ecmd->argv[0]);
    			}
    		}
    	}
    	
        break;
    ~~~

  - 重定向命令
  
    ~~~C
      case '>':
      case '<':
        rcmd = (struct redircmd*)cmd;
        
    	//关闭输入/输出文件（根据重定向类型决定）
    	close(rcmd->fd);
    
    	//open函数打开文件，第一个参数为对应文件，第二个参数为打开文件设置的fd编号，第三个参数为文件权限的八进制编码。返回值为0标志打开成功，否则为打开失败
    	//使用open函数，打开重定向文件，顶替关闭的输入/输出文件
    	if (open(rcmd->file, rcmd->flags, 0644) < 0){
    		fprintf(stderr, "Unable to open file: %s\n", rcmd->file);
    		exit(0);
    	}
    	
    	//运行命令
        runcmd(rcmd->cmd);
        break;
    
    ~~~
  
  - 管道命令
  
    管道命令的基本思想是首先创建一个管道，管道有一个读取端一个输出端。管道读取端连接一个输入子进程，将该输入子进程的输出文件设置为管道的读取端。管道输出端连接一个输出子进程，将该输出子进程的输入文件设置为管道的输出端。
  
    ~~~C
    case '|':
        pcmd = (struct pipecmd*)cmd;
        
    	//pipe函数在p空间中创造一个管道，得到两个fd的文件指针p[0]、p[1]分别代表管道的输出端和读取端。并且该进程的子进程的fd表也有用这两个指针。创建成功返回0，否则返回-1
    	if (pipe(p) < 0){
    		fprintf(stderr,"pipe created failed\n");
    		exit(0);	
    	}
    	
    	//创建一个输入子进程，用来向管道中输入数据，返回值0代表子进程
    	if (fork1() == 0){
    		close(1);	//关闭输入子进程的输出文件
    		dup(p[1]);	//dup函数将参数文件添加到fd表中，更新当前fd表空缺的最小编号位置。在这里1文件关闭，即为将管道的读取端设为进程的输出文件
    		close(p[1]);	//关闭子进程的管道读取端指针
    		close(p[0]);	//关闭子进程的管道输出端指针
    		runcmd(pcmd->left);		//运行管道左侧命令
    	}
    	
    	//创建一个输出子进程，用来从管道中读取数据，返回值0代表子进程
    	if (fork1() == 0){
    		close(0);	//关闭输出子进程的输入文件
    		dup(p[0]);	//将管道的输出端设置为进程的输入文件
    		close(p[0]);	//关闭子进程的管道读取指针
    		close(p[1]);	//关闭子进程的管道输出指针
    		runcmd(pcmd->right);	//运行管道右侧命令
    	}
    	
    	//父进程操作
    	close(p[0]);	//关闭父进程的管道输出指针
    	close(p[1]);	//关闭父进程的管道读取指针
    	wait(&r);		//等待输入子进程结束
    	wait(&r); 		//等待输出子进程结束
        break;
    ~~~
  
  ​    解题完成

