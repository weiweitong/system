# 4 文件和目录

## 4.2 函数 stat, fstat, fstatat, lstat

```c++
int stat(const char *restrict pathname, struct stat *restrict buf);
int fstat(int fd, struct stat *buf);
int lstat(const char *restrict pathname, struct stat *restrict buf);
int fstatat(int fd, const char *restrict pathname, struct stat *restrict buf, int flag);
// 四个返回值: 若成功返回0, 否则返回-1
```

一旦给出 pathname, 

- stat函数将返回与此命名文件有关的信息结构. 
- fstat 函数获得已在描述符 fd 上打开文件的有关信息. 

lstat 函数类似于 stat, 但是当命名的文件是一个符号链接时, lstat 返回该符号链接的有关信息, 而不是由该符号链接引用的文件的信息.

fstatat 函数为一个相对于当前打开目录 (由fd参数指向) 的路径名返回文件统计信息. flag 参数控制着是否跟随着一个符号链接. 当 AT_SYMLINK_NOFOLLOW标志被设置时, fstatat 不会跟随符号链接, 而是返回符号链接本身的信息. 如果 fd 参数的值是 AT_FDCWD, 并且 pathname 参数是一个相对路径名, fstatat 会计算相对于当前目录的 pathname 参数. 如果 pathname 是一个绝对路径, fd参数会被忽略. 根据 flag 的取值, fstatat 的作用就跟 stat 或 lstat 一样.

第二个参数 buf 是一个指针, 指向一个我们必须提供的结构. 函数来填充由 buf 指向的结构:

```c++
struct stat {
    mode_t				st_mode;	// file type & mode (permission)
    ino_t				st_ino;		// i-node number (serial number)
    dev_t				st_dev;		// device number (file system)
    nlink_t				st_nlink;	// number of links
    uid_t				st_uid;		// user ID of owner
    gid_t				st_gid;		// group ID of owner
    off_t				st_size;	// size in bytes, for regular files
    struct timespec		st_atime;	// time of last access
    struct timespec		st_mtime;	// time of last modification
    struct timespec		st_ctime;	// time of last file status change
    blksize_t			st_blksize;	// best I/O block size
    blkcnt_t			st_blocks;	// number of disk blocks allocated
};

// struct timespec 按照秒和纳秒定义了时间, 至少包括下面两个字段
struct timespec{
    time_t 	tv_sec;
    long 	tv_nsec;
};
```

使用 stat 函数最多的地方可能在于 ls -l 命令, 使其可以获得关于一个文件的所有信息.

## 4.3 文件类型

文件类型信息包含在 stat 结构的 st_mode 成员中:

- 普通文件(regular file), 不论是文本文件还是二进制文件, 对内核而言无区别, 但是UNIX系统必须理解二进制可执行文件的格式. S_ISREG()
- 目录文件(directory file) 包含了其他文件的名字以及指向这些文件有关信息的指针.  S_ISDIR()
- 块特殊设备(block special file) 提供对设备(如磁盘) 带缓冲的访问, 每次访问以固定长度为单位进行. S_ISCHA()
- 字符特殊设备(character special file). 这类型的文件提供对设备不带缓冲的访问. 每次访问的长度可变. S_ISBLK()
- FIFO, 这类型的文件用于进程间通信. 也称命名管道(named pipe). S_ISFIFO()
- 套接字(socket). 这类型的文件用于进程间的网络通信. S_ISSOCK()
- 符号链接(symbolic link) 指向另一个文件. S_ISLINK()

进程间通信(IPC) 对象可以说明为文件, 但与上述不同的是, 它们的参数储存在 stat 结构的指针

- 消息队列:   S_TYPEISMQ()
- 信号量:       S_TYPEISSEM()
- 共享存储对象:     S_TYPEISSHM()



## 4.4 设置用户 ID 和设置组 ID

与一个进程关联的 ID 有6个或更多:

- 实际用户ID, 实际组ID: 实际是谁
- 有效用户ID, 有效组, 附属组ID: 用于文件访问权限检查
- 保存的设置用户ID, 保存的设置用户组: 由 exec 函数保存.

每个文件有一个所有者和组所有者, 所有者由 stat 结构的 st_uid 指定, 组所有者则由 st_gid 指定.

## 4.5 文件访问权限

st_mode 也包含对文件的访问权限位. 每个文件有9个访问权限位, 可分为3类:

- 用户读, 写, 执行: S_IRUSR, S_IWUSR, S_IXUSR
- 组读, 写, 执行: S_IRGRP, S_IWGRP, S_IXGRP
- 其他读, 写, 执行: S_IROTH, S_IWOTH, S_IXOTH

术语用户指的是文件所有者(owner). 

chmod 命令用于修改这 9 个权限位. 该命令允许我们用 u 表示用户(所有者), 用 g 表示组, 用 o 表示其他.

三种读, 写, 执行, 都有:

- 用名字打开任一类型的文件时, 需该文件名中包含的每一个目录, 都需要具有执行权限: 例如, 为了打开文件/usr/include/stdio.h, 需要对目录/, /usr 和 /usr/include具有执行权限. 然后, 需要具有对文件本身的适当权限. 对目录的读权限与执行权限的意义是不同的:
  - 读权限允许我们读目录, 获得在该目录中所有文件名的列表. 
  - 执行权限允许我们通过该目录(也就是搜索该目录, 寻找一个特定的文件名).
- 读, 写权限决定我们能否读, 写该文件. 与open函数的O_RDONLY, O_WRONLY, O_RDWR 标志相关.
- 必须对一个文件有写权限, 才能在 open 函数中指定 O_TRUNC 标志.
- 在目录中新建一个文件, 必须对该目录有写权限和执行权限.
- 删除一个现有文件, 必须对该文件所在的目录有写权限和执行权限, 对该文件本身则不需要有读权限或写权限.
- 用 exec 函数执行文件, 必须对该文件有执行权限.

进程每次打开, 创建或删除一个文件时, 内核就进行文件访问权限测试, 涉及到文件的所有者(st_uid和st_gid), 进程的有效ID(有效用户ID和有效组ID), 以及进程的附属组ID. 进行如下的测试:

- 若进程的有效用户ID是 0 (超级用户), 则允许访问. 
- 若进程的有效用户ID等于文件的所有者ID, 那么如果所有者的的访问权限位被打开, 则允许访问, 否则拒绝.
- 组ID, 其他用户同上.

## 4.6 新文件或目录的所有权

而文件在新建时, 新文件或目录的用户ID设置为进程的有效用户ID

- 新文件的组ID可以是进程的有效组ID
- 新文件的组ID可以是它所在目录的组ID

## 4.7 函数 access 和 faccessat

当一个进程使用设置用户ID和设置组ID功能作为另一个用户(或组)运行时, 仍可能想验证其实际用户能否访问一个给定的文件. access 和 faccessat 函数是按照实际用户 ID 和实际组ID 进行访问权限测试的.

```c++
int access(const char *pathname, int mode);
int faccessat(int fd, const char *pathname, int mode, int flag);
/* 两个函数的返回值: 若成功, 返回0; 若出错, 返回-1
mode:
R_OK	测试读权限
W_OK	测试写权限
X_OK	测试执行权限
*/
```

## 4.8 umask 函数

```c++
mode_t umask(mode_t cmask);
/*
返回值: 之前的文件模式创建屏蔽字
用户可以设置 umask 值以限制他们所创建文件的默认权限:
400	用户读
200	用户写
100	用户执行
040	组读
020	组写
010	组执行
004	其他读
002	其他写
001	其他执行

常见的几种 umask 值是:
022	阻止其他写
022	阻止同组成员和其他用户写入
027	阻止同组成员写文件以及其他用户读,写或执行你的文件
*/
```

## 4.9 函数 chmod, fchmod 和 fchmodat

```c++
int chmod(const char *pathname, mode_t mode);
int fchmod(int fd, mode_t mode);
int fchmodat(int fd, const char *pathname, mode_t mode, int flag);
/*
三个函数返回值: 若成功, 返回0; 若出错, 返回-1
S_ISUID			执行时设置用户ID
S_ISGID			执行时设置组ID
S_ISVTX			保存正文(粘着位)

S_IRWXU			用户(所有者)读, 写和执行
	S_IRUSR		用户(所有者) 读
	S_IWUSR		用户(所有者) 写
	S_IXUSR		用户(所有者) 执行
	
S_IRWXG			组读, 写和执行
	S_IRGRP		组读
	S_IWGRP		组写
	S_IXGRP		组执行
	
S_IRWXO			其他读, 写和执行
	S_IROTH		其他读
	S_IWOTH		其他写
	S_IXOTH		其他执行
*/
```

- chmod 函数在指定的文件上进行操作, 
- fchmod 函数则对已打开的文件进行操作,
- fchmodat 函数与 chmod 函数在下面两种情况下是相同的: 
  - pathname 参数为绝对路径
  - fd 参数取值为 AT_FDCWD, 而 pathname 参数为相对路径
  - 否则, fchmodat 计算相对于打开目录(由 fd 参数指向) 的 pathname.  

flag 参数可用于改变 fchmodat 的行为, flag = AT_SYMLINK_NOFOLLOW 标志时, fchmodat 不会跟随符号链接.

为了改变一个文件的权限位, 进程的有效用户ID必须等于文件的所有者ID或者具有超级用户权限.

## 4.11 函数 chown, fchown, fchownat 和 lchown

```c++
int chown(const char *pathname, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int fchownat(int fd, const char *pathname, uid_t owner, gid_t group, int flag);
int lchown(cosnt char *pathname, uid_t owner, gid_t group);
/* 四个函数返回值: 若成功, 返回0; 若出错, 返回 -1

*/
```

除了所引用的文件是符号链接外, 这四个函数的操作类似. 在符号链接情况下, lchown 和 fchownat (设置了 AT_SYMLINK_NOFOLLOW 标志) 更改符号链接本身的所有者, 而不是该符号链接所指向文件的所有者. 

fchown 函数改变 fd 参数指向的打开文件的所有者, 既然指向一个已打开的文件, 不能用于改变符号链接的所有者.

## 4.14 文件系统

可以把一个磁盘分为一个或多个分区, 每个分区可以包含一个文件系统. i节点是固定长度的记录项, 它包含有关文件的大部分信息.

一个文件系统:	自举块, 超级块, 若干个柱面组(从n开始编号).

一个柱面组:		超级块副本, 配置信息, i节点图, 块位图, i节点数组, 数据块

一个i节点数组包含若干个i节点, 其后有若干个数据块和目录块. 

- 两个目录项指向同一个 i 节点, 每个 i 节点都有一个链接计数, 其值是指向该 i 节点的目录项数. 只有当链接计数减少至 0 时, 才可删除该文件(也就是释放该文件的数据块). 也就是为什么 "解除对一个文件的链接" 操作并不意味着 "释放该文件占用的磁盘块". 也是为什么删除一个文件的链接操作称为 unlink 而不是 delete 的原因. 在stat结构中, 链接计数包含在 st_nlink 成员中, 其基本系统数据类型是 nlink_t. 这种链接类型称为硬链接. 
- 另一种链接类型称为符号链接(symbolic link). 
- i 节点包含了文件有关的所哟信息: 文件类型, 文件访问权限位, 文件长度和指向文件数据块的指针. stat 结构中大多数信息都取自 i节点. 只有两项重要数据存放在目录项中: 文件名和 i 节点编号. i 节点编号的数据类型是 ino_t.
- 因为目录项中的一个 i 节点编号指向同一文件系统中的相应 i 节点, 一个目录项不能指向另一个文件系统的 i 节点. 这就是为什么 ln(1) 命令 (构造一个指向一个现有文件的新目录项) 不能跨越文件系统的原因. 
- 挡在不更换文件系统的情况下为一个文件重命名时, 该文件的实际内容并未移动, 只需要构造一个指向现有 i 节点的新目录项, 并删除老的目录项. 链接计数都不会改变. 例如, 将文件/usr/lib/foo 重命名为 /usr/foo, 如果目录项/usr/lib 和/usr 在同一文件系统中, 则文件 foo 的内容无需移动. 这就是 mv(1) 命令的通常操作方式.

## 4.15 函数 link, linkat, unlink, unlinkat 和 remove

任何一个文件都可以有多个目录项指向其 i 节点. 创建一个指向现有文件的链接的方法是使用 link 或 linkat 函数.

```c++
int link(const char *existingpath, const char *newpath);
int linkat(int efd, const char *existingpath, int nfd, const char *newpath, int flag);
/* 两个函数返回值: 若成功, 返回0, 若出错, 返回-1*/
```

link 引用 existingpath 指向 newpath. 
linkat 通过efd和 existingpath参数指定现有文件, 新路径名通过 nfd 和 newpath 参数指定.
默认情况下, 如果两路径名中有任意一个是相对路径, 则需要通过相对于对应的文件描述符进行计算. 
如果两个文件描述符中任何一个设置成 AT_FDCWD, 则相对路径名就相对于当前目录进行计算.

当现有文件是符号链接时, 由 flag 参数来控制:

- flag = AT_SYMLINK_FOLLOW, linkat 函数是创建指向现有符号链接目标的链接
- 如果这个标志被清除, 则创建一个指向符号链接本身的链接.

创建新目录项和增加链接计数应当是一个原子操作.

而为了删除一个当前的目录项, 可以调用unlink 函数

```c++
int unlink(const char *pathname);
int unlinkat(int fd, const char *pathname, int flag);
// 两个函数返回值: 若成功, 返回0; 若出错, 返回 -1 
```

这两个函数删除目录项, 并将由 pathname 所引用文件的链接计数减1. 

为了解除文件的链接, 必须对包含对该目录项的目录具有写和执行权限.

只有当链接计数为 0 时, 才可以删除该文件的内容. 只要有进程打开了该文件, 其内容也不能删除. 关闭一个文件时, 内核首先检查该文件的进程个数; 如果进程个数为0, 内核再去检查其链接计数, 如果为 0, 则删除该文件内容.

我们也可以用 remove 函数解除对一个文件或者目录的链接.对于文件, remove 功能与 unlink 相同. 对于目录, remove 功能与 rmdir 相同.

```c++
int remove(const char *pathname);
// 返回值: 若成功, 返回0; 若出错, 返回-1.
```

## 4.16 函数 rename 和 renameat

文件或目录都可以用 rename 函数 或 renameat 函数进行重命名. 

```c++
int rename(const char *oldname, const char *newname);
int renameat(int oldfd, const char *oldname, int newfd, const char *newname);
// 两个函数的返回值: 若成功, 返回0; 若出错, 返回-1
```

## 4.17 符号链接

引入符号链接是为了避免硬链接的一些限制:

- 硬链接通常要求链接与文件位于同一文件系统中
- 只有超级用户才能创建指向目录的硬链接

符号链接以及它所指向的对象并没有任何文件系统限制, 任何用户都可以创建指向目录的符号链接. 

符号链接一般用于将一个文件或者整个目录结构转移到系统中的另一个位置.

## 4.18 创建和读取符号链接

可以使用 symlink 或 symlinkat 函数创建出一个符号链接

```c++
int symlink(const char *actualpath, const char *sympath);
int symlinkat(const char *actualpath, int fd, const char *sympath);
// 两个函数的返回值: 若成功, 返回0; 若出错, 返回-1
```

函数创建了一个指向 actualpath 的新目录项 sympath. 

可以使用 readlink or readlinkat 打开链接本身, 并读取该链接中的名字:

```c++
ssize_t readlink(const char *restrict pathname, char *restrict buf, size_t bufsize);
ssize_t readlinkat(int fd, const char *restrict pathname, 
                   char *restrict buf, size_t bufsize);
// 返回值: 若成功, 返回读取的字节数; 若出错, 返回-1
```

## 4.20 函数 futimens, utimensat 和 utimes

```c++
int futimens(int fd, const struct timespec times[2]);
int utimensat(int fd, const char *path, const struct timespec times[2], int flag);
// 两个函数返回值: 若成功, 返回0; 若失败, 返回-1
```

futimens 和 utimensat 函数可以指定纳秒级精度的时间戳.

futimens 需要打开文件来更改它的时间. utimensat 函数提供了一种使用文件名更改文件时间的方法. 

## 4.21 函数 mkdir, mkdirat 和 rmdir

用 mkdir 和 mkdirat 函数创建目录

```c++
int mkdir(const char *pathname, mode_t mode);
int mkdirat(int fd, const char *pathname, mode_t mode);
// 返回值: 若成功, 返回0; 若出错, 返回-1
```

用 rmdir 函数删除空目录, 空目录是只包含 . 和 .. 这两项:

```c++
int rmdir(const char *pathname);
```

如果调用此函数, 没有进程打开该目录, 并且该目录的链接计数称为0, 则释放由此目录占用的空间. 

## 4.23 函数 chdir, fchdir, getcwd

每一个进程都有一个当前工作目录, 此目录是搜索所有相对路径名的起点(不以斜线为开始的路径名称为相对路径名). 

进程调用 chdir 或 fchdir 函数可以更改当前工作目录. 

getcwd 函数可以得到当前工作目录完整的绝对路径名.



