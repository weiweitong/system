## 1.1 导言

书读三遍, 方有余温.

为了更好地理解APUE, 我决定2020年再认真重读一遍并完成所有笔记, 预计是半个月内读完.

## 1.2 体系结构

Operating System 是一种控制计算机硬件资源和程序运行环境的软件. 通常称之为Kernel(内核).

Kernel的接口称之为system call(系统调用).

公用函数库构建在system call之上, 应用程序既可作公用函数库, 又可作为系统调用. 

## 1.3 登录

口令文件 /etc/passwd 中可以查看文件名, 其示例如下:

登录名:加密口令:数字用户ID:数字组ID:注释字段:起始目录:shell程序

sar:205:105:Stephen:/home/sar:/bin/ksh

shell 是一个命令行解释器, 它读取用户输入, 然后执行命令.

## 1.4 文件和目录

目录中的逻辑视图与实际磁盘存放位置不一定相同. 文件属性包括文件类型, 文件最后修改时间等, UNIX文件系统不一定在目录项中存放属性, 因为一个文件具有多个硬链接时, 很难做到多个属性副本之间同步.

## 1.5 输入与输出

文件描述符(file descriptor) 为一个小的非负整数. 当内核打开现有文件或者创建一个新文件时, 都返回一个文件描述符. 读写文件时, 都可以使用这个文件描述符.

每运行一个新程序时, shell都为其打开3个文件描述符, 即标准输入(standard input), 标准输出(standard output), 标准错误(standard output). 

## 1.6 程序和进程

程序是储存在磁盘中的可执行文件, 程序的执行实例被称为进程. 

UNIX确保每个进程都有一个唯一的数字标识符, 称为进程ID(process ID). getpid 获取进程ID.

三个用户进程控制的函数: 
- fork      以当前进程为父进程一次调用, 创建一个副本的子进程. 对父进程返回子进程的进程ID, 对子进程返回0. 一次调用, 两次返回.
- exec      在子进程中, 调用execlp替换子进程原先执行的程序文件, fork + execlp = spawn(产生新进程)
- waitpid   参数pid指定为要等待的进程, 返回子进程的终止状态(status变量), 可以用status判断子进程如何终止的

一个进程只有一个控制线程(thread). 所有线程共享同一地址空间, 文件描述符, 栈以及与进程相关的属性. 因为它们可以访问同一存储区, 所以各线程在访问共享数据时需要采取同步措施以避免不一致.

线程也具有ID, 但线程ID只在它所属的进程内起作用. 

## 1.7 出错处理

UNIX系统函数出错时, 通常返回一个errno负值, 例如出错返回-1. 有些函数出错使用另一种约定, 例如大多数返回指向对象指针的函数, 在出错时返回NULL指针.

errono 定义为一个符号, 扩展成一个可修改的左值(lvalue). 它可以是一个包含出错编号的整数, 也可以是一个返回出错编号指针的函数. 

扩展:
左值(lvalue): 其求值确定一个对象, 位域或函数的个体的表达式.
右值(rvalue): 计算某个运算数的值, 或初始化某个对象或位域后有结果对象.

### 1.7.1 多线程

在多线程环境中, 每个线程都有属于自己的局部errno以避免一个线程干扰另一个线程. 例如, Linux支持多线程存取errno:

```c++
extern int *__errno_location(void);
#define errno (*__errno_location())
```

- 只有函数返回值指示出错时, 才检验errno的值并由例程进行清除
- 任何函数都不会将errno值设置为0, 而<errno.h>中所有常量都不为0

### 1.7.2 出错恢复

致命性错误, 无法执行恢复动作, 最多用户屏幕打印出错消息或者将出错消息写入到日志文件

非致命性错误, 有时可以妥善处理, 大多数非致命性错误是暂时的(资源短缺), 当系统中活动较少时, 这种出错可能不会发生. 对资源型非致命性错误, 典型的操作是延迟一段时间, 然后重试.

## 1.8 用户标识

### 1.8.1 用户ID

用户ID(user ID) 是一个数值, 它向系统标识各个不同的用户. 系统管理员确定一个用户的登录名的同时确定其用户ID. 用户不能改变其唯一的用户ID.

用户ID为0的用户为根用户(root) 或超级用户(superuser). 

### 1.8.2 组ID

组ID也是由系统管理员在指定用户登录名时分配的, 允许同组的各个成员之间共享资源(如文件). 

## 1.9 Signal 信号

Signal用于通知进程发生了某种情况. 有以下三种处理信号的方式:
- 忽略信号: 不推荐, 如除0或者访问进程地址空间以外的存储单元等, 因为这些异常产生的后果不确定, 不能被忽略
- 按系统默认方式处理: 对于除0, 系统默认方式是终止该进程
- 提供一个函数: 信号发生时调用该函数, 称为捕捉信号. 通过自编的函数, 我们知道何时产生该信号, 并以期望的方式处理它.

### 1.9.1 中断信号

重温APUE的时候, 一下就想起了面试时被问过的一个问题, Linux是怎么杀进程的, kill怎么用, 背后的原理是什么?

那么在第一章中, 中断信号有三种不同的表现形式:
- interrupt key(中断键): Delete Ctrl+C
- quit key(退出键): Ctrl+\
- kill函数

前两种中断是在键盘上产生信号, 用于中断当前的进程. 对于shell中的程序, 如果按下中断键, 则系统默认操作是终止进程.

而一个进程可以调用kill函数向另一个进程发送信号, 发送者必须是进程的所有者或超级用户.

### 1.9.2 kill命令

通常, 对于一个前台进程(如shell进程), 可以使用键盘产生中断, 但是后台进程必须使用kill函数. 

我们需要首先使用 ps/pidof/pstree/top 等工具获取进程PID, 然后使用kill命令来杀掉该进程. Kill命令是通过向该进程发送指定的信号来结束相应的进程的. 

```bash
kill [参数] (进程名)

-l  信号，若果不加信号的编号参数，则使用“-l”参数会列出全部的信号名称
-a  当处理当前进程时，不限制命令名和进程号的对应关系
-p  指定kill 命令只打印相关进程的进程号，而不发送任何信号
-s  指定发送信号
-u  指定用户

kill 命令可以带信号号码选项, 也可以不带. 如果没有信号号码, kill命令就会发出终止信号(15), 这个信号可以被进程捕获, 使得进程在退出之前可以清理并释放资源. 

kill -2 123
等于前台运行123的进程的同时按下Ctrl+C, 普通用户最多可以使用到-9或者不带signal参数的kill命令.

只有第9种信号(SIGKILL)才可以无条件终止进程，其他信号进程都有权利忽略

HUP     1    终端断线
INT     2    中断（同 Ctrl + C）
QUIT    3    退出（同 Ctrl + \）
TERM   15    终止
KILL    9    强制终止
CONT   18    继续（与STOP相反， fg/bg命令）
STOP   19    暂停（同 Ctrl + Z）
```

### 1.9.3 killall命令

我们可以使用kill函数杀死杀死指定进程PID的进程, 如果需要找到我们需要杀死的进程, 我们还需要在之前使用ps等命令再配合grep来查找进程. 

killall指令把上述两条指令合并了, 直接杀死指定名字的进程, kill processes by name

```bash
killall [参数] (进程名)

-Z 只杀死拥有scontext 的进程
-e 要求匹配进程名称
-I 忽略小写
-g 杀死进程组而不是进程
-i 交互模式，杀死进程前先询问用户
-l 列出所有的已知信号名称
-q 不输出警告信息
-s 发送指定的信号
-v 报告信号是否成功发送
-w 等待进程死亡
--help 显示帮助信息
--version 显示版本显示

例如: 
1：杀死所有同名进程
    killall nginx
    killall -9 bash

2.向进程发送指定信号
    killall -TERM ngixn  或者  killall -KILL nginx
```

### 1.9.4 pkill命令

pkill命令可以按照进程名杀死进程.

```bash

pkill [选项] (参数)

-o：仅向找到的最小（起始）进程号发送信号；
-n：仅向找到的最大（结束）进程号发送信号；
-P：指定父进程号发送信号；
-g：指定进程组；
-t：指定开启进程的终端。

进程名称：指定要查找的进程名称，同时也支持类似grep指令中的匹配模式

pgrep -l gaim
2979 gaim

pkill gaim

```

## 1.10 时间

UNIX支持日历时间和进程时间.

在进程时间中, UNIX系统为一个进程维护了3个进程时间值:
- 时钟时间: wall clock time, 进程运行的时间总量, 值与系统中同时运行的进程数有关
- 用户CPU时间: 执行用户指令所用的时间量
- 系统CPU时间: 该进程执行内核程序所经历的时间

## 1.11 系统调用和库函数

### 1.11.1 系统调用

所有的操作系统都提供多种服务的入口点, 由此程序向内核请求服务. 这些入口点称为系统调用(system call). 

UNIX所使用的技术是为每一个系统调用在标准C库中设置一个具有同样名字的函数, 用户进程用标准C调用序列来调用这些函数. 然后, 该函数又根据系统的要求调用相应的内核服务. 

例如, 函数可以将一个或多个C参数送入通用寄存器, 然后执行某个产生软中断进程内核的机器指令. 从应用角度, 可以将系统调用视为C函数.

### 1.11.2 库函数

虽然库函数可能会调用一个或多个内核的系统调用, 但是它们并不是内核的入口点. 例如, printf函数会调用write系统调用以输出一个字符, 但函数strcpy(复制一个字符串) 和 atoi(将ASCII转换成整数) 并不使用任何内核的系统调用.



