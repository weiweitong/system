# 3 文件I/O

## 3.1 文件I/O 快速导读

大多数文件I/O只需要5个函数, oepn, read, write, lseek以及close.

本章中用到的文件I/O多为不带缓冲的I/O. 不带缓冲是指每个read和write都调用内核中一个系统调用. 

只要涉及在多个进程间共享资源, 原子操作的概念就变得非常重要. 

## 3.2 文件描述符

对于内核, 所有打开的文件都通过文件描述符引用. 当打开或创建一个文件时, 内核向进程返回一个文件描述符. 

UNIX系统shell将文件描述符0与进程标准输入管理, 文件描述符1与标准输出关联, 文件描述符2与标准错误关联. 

而在POSIX标准中, 应将其替换为STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO以提高可读性. 

## 3.3 函数open和openat

```c++
#include <fcnt1.h>
int open(const char *path, int oflag, ... /* mode_t mode */ );
int openat(int fd, const char *path, int oflag, ... /* mode_t mode */ );
```

最后一个参数写成..., 表明余下参数的数量与类型是可变的.

对于open函数而言, 仅当创建新文件时才使用这个参数. 

path参数是要打开或者创建文件的名字, oflag参数可用来说明此函数的多个选项. 用下列一个或多个常量进行"或"运算构成oflag参数. 

|  oflag参数  |                        oflag参数意义                         |
| :---------: | :----------------------------------------------------------: |
|  O_RDONLY   |                      只读打开, 定义为0                       |
|  O_WRONLY   |                      只写打开, 定义为1                       |
|   O_RDWR    |                     读, 写打开, 定义为2                      |
|   O_EXEC    |                          只执行打开                          |
|  O_SEARCH   |                    只搜索打开(应用于目录)                    |
|             |                   以上五个常量有且仅有一个                   |
|             |                         以下常量可选                         |
|  O_APPEND   |                   每次写时加入到文件的尾部                   |
|  O_CLOEXEC  |             把FD_CLOEXEC常量设置为文件描述符标识             |
|   O_CREAT   | 若此文件不存在则创建它, open函数创建时需同时说明第三个mode参数, 用mode指定该新文件的访问权限位 |
| O_DIRECTORY |                如果path引用的不是目录, 则报错                |
|   O_EXCL    | 如果同时指定了O_CREAT, 而文件以存在, 则出错. 用此可以测试一个文件是否存在, 如果不存在, 则创建此文件. 这使得测试与创建两者合并为一个原子操作. |
|  O_NOCTTY   | 如果path引用的是终端设备, 则不将该设备分配作为此进程的控制终端. |
| O_NOFOLLOW  |            如果path引用的是一个符号链接, 则出错.             |
| O_NONBLOCK  | 如果path引用一个FIFO, 块特殊文件或字符特殊文件, 此选项为文件的本次打开操作和后续的I/O操作设置非阻塞方式. |
|   O_SYNC    | 使每次write等待物理I/O操作完成, 包括由该write操作引起的文件属性更新所需的I/O. |
|   O_TRUNC   |  如果此文件存在, 且为只写或读写成功打开, 则将其长度截断为0   |
| O_TTY_INIT  | 如果打开一个还未打开的终端设备, 设置非标准termios参数, 使其符合Single UNIX Specification. |
|   O_DSYNC   | 使每次write要等物理I/O操作完成, 但如果该写操作并不影响读取刚写入的数据, 则不需等待文件属性被更新. |
|   O_RSYNC   | 使每一个以文件描述符作为参数进行的read操作等待, 直至所有的对文件同一部分挂起的写操作都完成. |
|   O_FSYNC   |                          等待写完成                          |
|   O_ASYNC   |                           异步 I/O                           |

fd参数把open和openat函数区分开, 共有三种可能性:

- path参数指定的是绝对路径名, 在这种情况下, fd参数被忽略, openat函数就相当于open函数
- path参数指定的是相对路径名, fd参数指出了相对路径名在文件系统中的开始地址.
- path参数指定了相对路径名, fd参数具有特殊值AT_FDCWD. 此时路径名从当前工作目录中获取, openat函数在操作上与open函数类似.

openat是POSIX.1 新增函数, 主要解决:

- 让线程可以使用相对路径名打开目录中的文件, 而不再只能打开当前工作目录
- 可以避免 time-of-check-to-time-of-use (TOCTTOU) 错误.

TOCTTOU 错误的基本思想是: 如果两个基于文件的函数调用, 其中第二个调用依赖于第一个调用的结果, 那么程序是脆弱的. 因为两个调用并不是原子操作, 如果两个函数调用期间文件改变了, 也造成了第一个调用的结果不再有效. 使得程序最终结果出错.

## 3.4 函数 creat

```c++
// 也可以调用 creat 函数创建一个新文件
int creat(const char *path, mode_t mode);
// 该函数等价为:
open(path, O_WRONLY|O_cREAT|O_TRUNC, mode);
```

creat不足在于它只能以只写方式打开所创建的文件. 之前, 创建并读取一个文件, 必须先调用creat, close, 然后再调用open. 现在可用下列方式调用open实现:

```c++
open(path, O_RDWR|O_CREAT|O_TRUNC, mode);
```

## 3.5 函数close

```c++
// 可调用close函数关闭一个打开文件
int close(int fd);
// 若成功返回0, 若失败返回1
```

关闭一个文件时还会释放该进程加在该文件上的所有记录锁.

当一个进程终止时, 内核自动关闭它所有的打开文件.

## 3.6 函数lseek

每个打开文件都有一个与其相关联的"当前文件偏移量"(current file offset), 表示从文件开始处计算的字节数.

读, 写操作都从当前文件偏移量处开始. 并使偏移量增加所读写的字节数. 打开一个文件时, 按系统默认情况, 除非指定O_APPEND 选项, 否则该偏移量被设置成0. 

```c++
// 可以用 lseek 显式地为一个打开文件设置偏移量.
off_t lseek(int fd, off_t offset, int whence);
// 返回值:	若成功, 返回最新的文件偏移量; 若出错, 返回为 -1
```

- 若whence是SEEK_SET, 则将该文件的偏移量设置为距文件开始处offset个字节
- 若whence是SEEK_CUR, 则将该文件的偏移量设置为当前值加offset, offset可正可负
- 若whence是SEEK_END, 则将该文件的偏移量设置为文件长度加offset, offset可正可负

若lseek成功执行, 则返回新的文件偏移量, 为此可以用下列方式正确打开文件的当前偏移量:

```c++
off_t currpos;
currpos = lseek(fd, 0, SEEK_CUR);
```

这种方法也可以用来确定所涉及的文件是否可以设置偏移量. 如果文件描述符所指向的是一个管道, FIFO 或 网络套接字, 则lseek返回 -1, 并将errno 设置为ESPIPE.

```c++
// 测试标准输入能否设置偏移量:
int main(void) {
    if (lseek(STDIN_FILENO, 0, SEEK_CUR) == -1)
        printf("cannot seek and cannot set offset");
    else
        printf("seek success");
    exit(0);
}
```

lseek仅将当前的文件偏移量保存在内核中, 不涉及任何的I/O操作. 然后, 该偏移量用于下一个读或者写操作.

文件偏移量可以大于当前文件长度, 在这种情况下, 对该文件的下一次写将加长该文件, 并在文件中构成一个空洞. 位于文件中但没有被写过的字节都被读为0.

文件中的空洞并不要求在磁盘上占用存储区. 具体处理方式与文件系统的实现有关. 当定位到超出文件尾端之后写时, 对于新写入的数据需要分配磁盘块, 但是对于原文件尾端和新开始写位置之间的部分则不需要分配磁盘块.

```c++
char buf1[] = "abcdefghij";
char buf2[] = "ABCDEFGHIJ";

int main() {
    int fd;
    if ((fd = creat("file.hole", FILE_MODE)) < 0)
        err_sys("creat error");
    
    if (write(fd, buf1, 10) != 10) 
        err_sys("buf1 write error");
    // offset now is 10
    
    if (lseek(fd, 16384, SEEK_SET) == -1) 
        err_sys("lseek error");
    // offset now is 16384
    
    if (write(fd, buf2, 10) != 10)
        err_sys("buf2 write error");
    // offset nos is 16394
    
    exit(0);
}
```

## 3.7 函数 read

```c++
// 调用 read 函数从打开文件中读取数据, 从当前偏移量处开始读
ssize_t read(int fd, void *buf, size_t nbytes);
// 返回值: 读到的字节数, 若已到文件尾, 返回0, 若出错, 返回-1
```

## 3.8 函数 write

```c++
// 调用 write 函数向打开文件写数据, 从文件的当前偏移量处开始写
ssize_t write(int fd, const void *buf, size_t nbytes);
// 返回值: 若成功, 返回已写的字节数, 若出错, 返回-1
```

write出错的常见原因是:

- 磁盘被写满
- 超过一个给定进程的文件长度限制

## 3.9 I/O的效率

I/O的 BUFFSIZE 值跟磁盘有关. 比如Linux 的 ext4 文件系统, 其磁盘块长度是4096字节. 因此缓冲区大于4096后继续增加并不能减少CPU时间.

文件系统为了改善系统性能都采用某种预读(read ahead)技术. 当检测到正进行顺序读取时, 系统就试图读入比应用所要求的更多数据, 并假想应用很快就会读这些数据.

## 3.10 文件共享

UNIX 系统支持在不同进程间共享打开文件. 

内核使用 3 种数据结构表示打开文件, 它们之间的关系决定了在文件共享方面一个进程对另一个进程可能产生的影响. 

1. 每个进程在进程表中都有一个记录项, 记录项中包含一张打开文件描述符表, 可视为一个矢量, 每个描述符占一项. 与每个文件描述符相关联的是
   - 文件描述符标志 (close_on_exec)
   - 指向一个文件表项的指针

2. 内核为所有打开文件维持一张文件表, 每个文件表项包含:
   - 文件状态标志(读, 写, 添写, 同步和非阻塞)
   - 当前文件偏移量
   - 指向该文件 v 节点表项的指针

3. 每个打开文件(或设备) 都有一个 v 节点(v-node) 结构. v 节点包含了文件类型和对此文件进行各种操作函数的指针. 对于大多数文件, v 节点还包含了该文件的 i 节点(i-node, 索引节点). 这些信息是在打开文件时从磁盘读入内存的, 所以, 文件的所有相关信息都是随时可用的. 例如, i节点包含文件的所有者, 文件长度, 指向文件实际数据块在磁盘上所在位置的指针等.  (Linux 没有使用 v 节点, 而是通过 i 节点的结构. 虽然两种实现方式不同, 但在概念上 v 节点和 i节点一样的. 两者都是指向文件系统特有的 i 节点结构. )

<img src="C:\Users\rust\Desktop\APUE\chapter3\img\OpenFileDataStructure.jpg" alt="打开文件的内核数据结构" style="zoom:50%;" />

如果两个独立进程各自打开了同一文件, 则有如下图所示的关系:

<img src="C:\Users\rust\Desktop\APUE\chapter3\img\MultiProcessorOpenOneFile.jpg" alt="MultiProcessorOpenOneFile" style="zoom:50%;" />

可以看出, 每个进程都获得各自的一个文件表项, 但对一个给定的文件只有一个 v 节点表项. 这样每个进程都可以设置自己的当前偏移量.

- 完成每个write后, 在文件表项中的当前文件偏移量即增加所写入的字节数. 如果这导致当前文件偏移量超出了当前文件长度, 则将 i 节点表项中的当前文件长度设置为当前文件偏移量
- 如果用O_APPEND 标志打开一个文件, 则相应标志也被设置到文件表项的文件状态标志中. 每次对这种具有追加标志的文件执行写操作时, 文件表项中的当前文件偏移量首先被设置成 i 节点表项中的文件长度. 这样使得每次写入的数据都追加到文件的当前尾端处.
- 若一个文件用 lseek 定位到文件当前的尾端, 则文件表项中的当前文件偏移量被设置成 i 节点表项中的当前文件长度. 
- lseek 函数只修改文件表项中的当前文件偏移量, 不进行任何 I/O 操作.

可能有多个文件描述符指向同一文件表项. 3.12 的 dup函数即可看到这一点. 另外 fork 后也会发生同样的情况, 此时父进程, 子进程各自的每一个打开文件描述符共享同一个文件表项.

文件描述符只用于一个进程的一个描述符, 而文件状态标识符则应用于指向该文件给定文件表项的任何进程中的所有描述符. 

## 3.11 原子操作

### 3.11.1 追加到一个文件

若有多个进程同时使用这种方法将数据追加写到同一文件, 就会产生问题(例如, 若此程序由多个进程同时执行, 各自将消息加到同一日志文件中, 就会产生这种情况.)

问题出在逻辑操作 "先定位到文件尾端, 然后写". 它使用了两个分开的函数调用. 解决方法是使这两个操作对于其他进程而言成为一个原子操作. 任何要求多于一个函数调用的操作都不是原子操作, 因为在两个函数调用之间, 内核可能会临时挂起进程.

UNIX系统为这样的操作提供了一种原子操作的方法, 即在打开文件时设置 O_APPEND 标志. 这样使得内核在每次写操作前, 都将进程的当前偏移量设置到该文件的尾端处, 于是每次写之前就不再需要调用 lseek . 

### 3.11.2 函数 pread 和 pwrite

XSI扩展允许原子性地定位并执行 I/O. pread 和 pwrite 就是这样的扩展.

```c++
#include <unistd.h>
ssize_t pread(int fd, void *buf, size_t nbytes, off_t offset);
// 返回值: 读到的字节数, 若已到文件尾, 返回0, 若出错, 返回 -1

ssize_t pwrite(int fd, const void *buf, size_t nbytes, off_t offset);
// 返回值: 若成功, 返回已写的字节数; 若出错, 返回 -1
```

调用 pthread 相当于调用 lseek 后调用 read, 但 pread 与这种顺序调用有重要区别:

- 调用 pread 时, 无法中断其定位和读操作
- 不更新当前文件偏移量.

调用 pwrite 相当于调用 lseek 后调用 write, 但也有上述类似的区别.

### 3.11.3 创建一个文件

对 open 函数的 O_CREAT 和 O_EXCL 选项进行说明时, 已经提到了原子操作. 

检查文件是否存在和创建文件这两个操作是作为一个原子操作执行的. 

如果没有这样一个原子操作, 如果在open和creat之间, 另一个进程创建了该文件, 并写入了一些数据, 然后, 原先进程执行这段程序的creat, 刚由另一个进程写入的数据就会被擦去. 

原子操作(atomic operation) 指的是由多步组成的一个操作. 如果该操作原子地执行, 则要么执行全部完成步骤, 要么一步也不执行, 不可能只执行所有步骤中的一个子集. 

## 3.12 函数dup和dup2

下面两个函数都可以用来复制现有的一个文件描述符

```c++
int dup(int fd);
int dup2(int fd, int fd2);
// 两函数的返回值: 若成功, 返回新的文件描述符; 若出错, 返回 -1
```

由dup返回的新描述符一定是当前可用描述符中的最小数值. 

对于 dup2, 可用用 fd2 参数指定新描述符的值.

- 如果 fd2 已经打开, 则先将其关闭. 
- 如若 fd 等于 fd2, 则 dup2 返回 fd2, 而不是关闭它.

这些函数返回的新文件描述符与参数 fd 共享同一个文件表项.

![After-dup-Kernel-dataStructure](C:\Users\rust\Desktop\APUE\chapter3\img\After-dup-Kernel-dataStructure.jpg)

复制一个描述符的另一种方法是使用 fcnt1 函数.  实际上:

```c++
调用 
    dup(fd); 
等效于调用:
fcnt1(fd, F_DUPFD, 0);

而调用
    dup2(fd, fd2);
并不等效于, 因为dup2是原子操作
close(fd2);
fcnt1(fd, F_DUPFD, fd2);
```

## 3.13 函数 sync, fsync 和 fdatasync

向文件写入数据时, 内核先将数据复制到缓冲区, 然后排入队列, 晚些再写入磁盘. 称为延迟写(delayed write).

- sync 将所修改过得块缓冲区排入写队列, 然后就返回, 并不等待实际写磁盘操作结束.
- fsync 函数只对由文件描述符 fd 指定的一个文件起作用, 并且等待写磁盘操作结束后才返回.
- fdatasync 类似于 fsync, 但只影响文件的数据部分. 而除数据以外, fsync 还会同步更新文件的属性.

## 3.14 函数 fcnt1

fcnt1 函数可以改变已经打开文件的属性.

```c++
int fcnt1(int fd, int cmd, ... /* int arg */);
// 返回值: 若成功, 则依赖于cmd, 若出错, 返回 -1
```

- cmd = F_DUPFD 或 F_DUPFD_CLOEXEC 复制一个已有的描述符
- cmd = F_GETFD 或 F_SETFD, 获取/设置 文件描述符标志
- cmd = F_GETFL 或 F_SETFL, 获取/设置 文件状态标志
- cmd = F_GETOWN or F_SETOWN, 获取/设置 异步I/O所有权
- cmd = F_GETLK, F_SETLK or F_SETLKW, 获取/设置 记录锁

## 3.15 函数 ioctl

I/O操作的杂物箱. 终端I/O是使用ioctl最多的地方.

