

# 1. HSM引言

Lustre HSM（Hierarchical Storage Management）是lustre文件系统提供的一种多层级存储管理功能。该功能使得可以将lustre绑定到一个或者多个下级存储系统（以下简称HSM存储），从而使得可以将lustre作为一个上层的高速缓存进行使用。 

lustre所提供的HSM主要架构如下： 

![HSM架构](img/hsm架构.png ''HSM架构'')

lustre的每个mdt都可以启动一个hsm服务，该服务又可称为Coordinator，负责收集客户端发起的hsm请求，并按照一定的策略将该请求转发到对应的Agent。 

Agent是运行在lustre客户端上的一个或者多我之前个用户进程。Agent负责接收来自Coordinator的hsm请求，并将请求解析请求，向具体的Copy tool发送HSM命令。 

Copytool则负责执行具体的hsm操作。Copytool仅响应来自于Agent的命令，其本质上是一个用户态进程。Copytool是一个针对lustre和hsm存储专用设计的数据迁移工具，例如，如果hsm后端是对象存储，提供S3接口，则Copytool需要设计成lustre的posix-to-s3的数据迁移工具。Copytool在具体执行时可以做一些其他操作，比如压缩、去重以及加密等。 

HSM架构可以设置一个集中式的策略引擎PolicyEngine。PolicyEngine是一个运行在lustre客户端的后台服务，负责实时监控lustre mdt的changelog等其他信息，之后根据用户指定的具体策略，向lustre发送HSM命令。 

![HSM Policy Engine](img/hsm.png ''HSM Policy Engine'')

当前针对lustre且工业上较为成熟的Policy Engine是Robinhood



# 2. HSM支持的操作

每个HSM Agent均需要向cdt注册。注册agent时需要指定--archieve参数。Lustre HSM功能支持最多32个带有编号的agent ID，编号从1开始。另外支持一个特殊的agent，该agent的编号为0，表示该agent可以用于服务所有的hsm后端。每个ID对应的agent可以注册无穷多个，lustre coordinator会根据一定的策略从所注册的agent中选出一个最适合的，之后向其发送hsm request。 

HSM的命令由客户端发起，具体的命令有以下几种： 

```c++
    hsm_archive [--archieve=ID] FILE1 [FILE2…] 
    // 复制filelist中的文件下沉到HSM存储中，如果不指定--archieve，则默认使用ID=0

    hsm_release FILE1 [FILE2…] 
    // 删除lustre中对应该文件的数据部分。执行之前确保数据已被archive到HSM中

    hsm_restore FILE1 [FILE2…] 
    // 从HSM中恢复文件列表中的数据到lustre中

    hsm_cancel FILE1 [FILE2…] 
    // 取消针对文件列表正在执行的HSM请求
    
    hsm_action FILE […] 
    // 显示针对文件列表中的文件正在执行的hsm请求

    hsm_state  FILE […] 
    // 显示文件列表中的HSM状态
```

当前支持的状态有以下几种:

| state | explanation | annotation |
| :--: | :---: | :---: |
| Non-archive | 标识为该状态的文件将不会被archive | |
| Non-release | 标识为该状态的文件将不会被release  | |       
| Dirty | 存储在lustre中的文件以被更改，需要执行archive操作以便更新HSM中的副本 | |
| Lost | 该文件已被archive，但是存储的HSM中的副本已丢失 | 打该标签时需要以root权限 |
| Exist  | 文件的数据拷贝在HSM中已经存在 | |
| Rleased | 文件的LOV信息以及LOV对象已从lustre中移除 | |
| Archived    | 文件数据已整个从lustre中拷贝至HSM中  | |




```c++
    hsm_set  [FLAGS] FILE […] 
    // 将文件列表中的文件打上上述标签
    
    hsm_clear [FLAGS] FILE […] 
    // 移除文件列表中相应的标签

    hsm_import path [flags…] 
    // 在lustre中创建文件存根，路径为path，
    // 其他信息由flags指定。文件的hsm状态为released，
    // 即文件的内容存储在后端存储，
    // 仅当文件被打开或者用户主动调用restore时文件的内容才被加载
```
 



# 3. HSM操作流程

## 3.1. 文件的状态的转换

一个HSM请求总是由lustre客户端发起。当客户端创建一个新的文件时，可以执行archive操作，此时该文件将被拷贝到HSM后端存储，并被标记为archived状态。当客户端针对该文件执行release操作时，文件的数据将从lustre中删除，此时lustr仅存储该文件的元数据信息且文件将被标记为released状态。当客户端针对该文件执行restore操作时，该文件的数据将从hsm中拷贝到lustre中，且文件的状态被标记为archived状态。当lustre中的文件被修改时，文件将被标记为dirty状态，此时客户端需要再次执行archive操作，以保证HSM中的数据与lustre中的数据同步。 

![文件的状态的转换](img/file.png ''文件的状态的转换'')

## 3.2. Lustre的HSM架构设计

lustre的HSM旨在提供一套多层存储的操作接口，并不提供接口的具体实现。在lustre的HSM中，文件的数据可以从lustre的OST中迁移到HSM存储中，但是文件的元数据信息，如文件名、用户ID、权限信息等将会一直保存在lustre的MDT中。当客户端打开一个已被release的文件或者主动执行restore操作时，lustre将自动从HSM中重新加载文件对应的数据。restore的整个过程如如下：

![HSM架构设计](img/lustreHSM.png ''HSM架构设计'')

客户端打开或者主动执行restore操作，该请求将首先被发送到相关的MDT服务中。MDT检测到文件的相应数据不在lustre中，将首先在OST中为该文件分配对应的存储空间，之后将文件的FID发送到HSM的CDT服务中。CDT服务解析该请求，并将该请求进一步封装发送到注册到该MDT中的合适的agent中。agent服务中产生对应的HSM事件，并从事件中解析到请求类型以及请求关联的FID。agent服务之后将请求进一步封装并发送到对应的Data Mover进程中。Data Mover服务具体的数据搬运工作，同时还可以进一步做一些数据的压缩、去重、加密等操作。在数据的搬运过程中，Data Mover可以向agent汇报数据搬运的进度，此时agent亦可将该进度汇报给CDT服务。数据搬运完成后Data Mover将向agent汇报数据搬运完成事件，之后agent将该事件进一步封装并汇报给MDT服务。MDT收到数据搬运完成的事件后将通知client文件打开成功。至此整个文件的restore完成。在此期间，客户端将一直处于阻塞状态。 

更一般的hsm请求控制流如下: 

    1. 客户端调用llapi_hsm_request()创建hsm请求。 
    2. lustre client通过mdc_ioc_hsm_request()将该请求发送到mdt。 
    3. mdt创建相应的hsm请求日志llog，并唤醒cdt服务。 
    4. cdt服务被唤醒，从llog读取对应的日志内容，创建具体的HSM事件，并将事件发送到agent所在的客户端。 
    5. agent接收到hsm事件，并处理事件中封装的具体hsm请求
    6. agent可以随时调用llapi_hsm_action_progress()向CDT汇报数据搬运的进度。 
    7. 数据搬运完成后agent调用llapi_hsm_action_end()用于汇报mdt数据已搬运完成。 


## 3.3. Lustre MDT提供的HSM调节参数

lustre的mdt参数项中包含一些关于hsm的参数。参数路径为/proc/fs/lustre/mdt/<fsname>/ 

| 参数项 | 属性 | 释义 | 备注 | 
| :--: | :---: | :---: | :---: |
hsm_control |  rw | 控制cdt服务的启停，可选配置如下： <br> enabled:    开启cdt服务线程   <br> disabled:   暂停cdt一切服务  <br>  shutdown: 关闭cdt服务，释放所有cdt资源  <br> purge:        清除所有的hsm请求 | 
hsm/actions | r  |                                                          
hsm/active_request_timeout | rw | 一个active请求执行的最大超时时间 |
hsm/active\_requests | r | |
hsm/agents | r | 查看当前已经注册的所有agent | 
hsm/archive\_count | r | 当前已经执行archive的统计计数  |
hsm/default\_archive\_id | rw | cdt 默认使用的archive ID |
hsm/grace\_delay |  rw | 一个成功的或者失败的hsm请求被从请求列表中移除的最长时间   |
hsm/group\_request\_mask | rw | 组访问权限相关 |
hsm/user\_request\_mask | rw | 用户访问权限相关 |
hsm/loop\_period | rw | hsm CDT扫描llog的周期 |
hsm/max\_requests  | rw | 每个mdt最大的active请求数 |
hsm/policy | rw | hsm处理策略, 有以下两种：  <br>  NRA：No Retry Action, 如果restore失败，则不会再次执行   <br>  NBR：Non Blocking Restore，如果访问一个已经被released的文件，将不会出发自动restore操作。 |
hsm/remove\_archive\_on\_last\_unlink | rw | lustre文件中的数据被rm后是否发出hsm请求删除hsm存储中对应的数据。        |
hsm/remove\_count | r  | |
hsm/restore\_count | r | |

---
# 4. Lustre的HSM接口
lustre向外部导出C语言的API接口，主要的接口定义如下:

## 4.1. 获取HSM状态

### 4.1.1. HSM状态

这个struct是为了显示当前Lustre files状态，描述了当前的HSM状态和进程中的行为。

其结构与成员描述如下：

```c++
    struct hsm_user_state {
		/* hus_states和hus_archive_id表示当前HSM状态 */
		// hus状态
		__u32			hus_states;     
		// hus归档ID
		__u32			hus_archive_id; 
		/*  若当前存在 HSM 行为 
			则用如下的四个元素描述该行为:
		*/
		// hus进程中状态
		__u32			hus_in_progress_state;  
		// hus进程中行为
		__u32			hus_in_progress_action; 
		// hus进程中位置
		struct hsm_extent	hus_in_progress_location;   
		// hus扩展信息
		char			hus_extended_info[];    
	};
```



### 4.1.2. 通过文件标识符获取hsm状态

```c++
    /*  此API的目的是返回HSM状态和HSM请求
    API调用方给出一个文件标识符fd
    和一个hsm_user_state结构的指针hus
    然后通过hus指针返回该HSM状态。*/
    int llapi_hsm_state_get_fd(int fd, struct hsm_user_state *hus);
```

若该API返回int值为0则表示操作成功，为负则报错，负值为出错码。

```c++
    /* 此API中，主要是调用对I/O通道进行管理的ioctl函数 */
    rc = ioctl(fd, LL_IOC_HSM_STATE_GET, hus);
```

而传入该ioctl函数中的为文件标识符fd\\和CMD命令LL\_IOC\_HSM\_STATE\_GET，此CMD命令参数为hus

```c++
    #define LL_IOC_HSM_STATE_GET_IOR('f', 211, struct hsm_user_state)
    /*  如上的define语句可看出，
    LL_IOC_HSM_STATE_GET命令实际为IO read命令
    传入ioctl的参数hus的作用为传回一个HSM状态 */
```


### 4.1.3. 通过文件路径获取HSM状态

```c++
    /* 此API的目的是返回HSM状态和HSM请求
    API调用方给出一个文件路径path
    和一个hsm_user_state结构的指针hus
    然后通过hus指针返回该HSM状态。 */
    int llapi_hsm_state_get(const char *path, struct hsm_user_state *hus);
```

而此API的具体实现是：

```c++
    // 先通过路径名获取fd文件标识符 
    fd = open(path, O_RDONLY | O_NONBLOCK);
    // 再调用上述的get_fd函数获取hsm状态
    rc = llapi_hsm_state_get_fd(fd, hus);
```

## 4.2. 设置HSM状态

### 4.2.1. 通过文件标识符设置hsm状态

```c++
/* 此API的目的是通过文件标识符fd设置HSM状态*/
int llapi_hsm_state_set_fd(int fd, __u64 setmask, __u64 clearmask, __u32 archive_id);
```

该函数会新建一个如下的hsm state set结构的指针hss：

```c++
struct hsm_state_set {
	__u32	hss_valid;
	// hss_valid初始设置为0x03
	__u32	hss_archive_id;
	/*  如果传入的archive_id大于0,则hss_valid设置为0x07, 
	并且hss_archive_id设置为传入archive_idhss_valid */
	__u64	hss_setmask;
	// hss_setmark设置得与传入参数一致
	__u64	hss_clearmask;
	// has_clearmask设置得与传入参数一致
};
```


之后调用ioctl函数：

```c++
	rc = ioctl(fd, LL_IOC_HSM_STATE_SET, &hss);
	// 其中CMD命令为：
	LL_IOC_HSM_STATE_SET
	// 此命令实际为：
	#define LL_IOC_HSM_STATE_SET		_IOW('f', 212, struct hsm_state_set)
	// 它实际上使用IO write方法
	// 将hss指向的的hsm state set写入文件标识符fd所指的文件中
```


### 4.2.2. 通过文件路径设置hsm状态

```c++
	/* 此API的目的是通过文件路径path设置HSM状态*/
	int llapi_hsm_state_set(const char *path, __u64 setmask, __u64 clearmask, __u32 archive_id);
```

此API中重要步骤为：

```c++
	/* 此函数通过open函数打开文件, 找到文件标识符fd */
	fd = open(path, O_WRONLY | O_LOV_DELAY_CREATE | O_NONBLOCK);
	/* 然后调用3.1中的函数修改hsm状态 */
	rc = llapi_hsm_state_set_fd(fd, setmask, clearmask, archive_id);
```

### 4.2.3. HSM日志报错

```c++
    /* 此API的目的是通过文件标识符fd设置HSM状态
    当一段监控FIFO队列被注册后，自定义的日志回调被调用。
    将日志条目格式化成Json事件格式，这样可以被copytool监控进程所消费。*/
    void llapi_hsm_log_error(enum llapi_message_level level, int _rc, const char *fmt, va_list args);
```
此API的输入为：
```c++
    // enum的llapi_message_level有几种值
    enum llapi_message_level {
        LLAPI_MSG_OFF    = 0,   // OFF状态位为0
        LLAPI_MSG_FATAL  = 1,   // FATAL状态位为1
        LLAPI_MSG_ERROR  = 2,   // ERROR状态位为2
        LLAPI_MSG_WARN   = 3,   // WSRN状态位为3
        LLAPI_MSG_NORMAL = 4,   // NORMAL状态位为4
        LLAPI_MSG_INFO   = 5,   // INFO状态位为5
        LLAPI_MSG_DEBUG  = 6,   // DEBUG状态位为6
        LLAPI_MSG_MAX
    };

    int _rc // rc设置返回消息的代码
    const char *fmt  // fmt格式化字符串
    va_list args  // args待格式化的字符串
```

## 4.3. HSM事件队列

### 4.3.1. 注册HSM事件队列

```c++
    /* 此API的目的是通过文件路径path注册HSM事件队列
    该事件队列是为了监控进程可以从中读取
    而在没有读取器读取时，写入事件将丢失
    */
    int llapi_hsm_register_event_fifo(const char *path);
```
此API的重要实现为：
```c++
    /* 从路径中读取(若读取不存在则新建)一个FIFO */
    mkfifo(path, 0644)
    /* 先以读取打开文件, 为了防止写指令不会马上失效 */
    read_fd = open(path, O_RDONLY | O_NONBLOCK);
    /*  再将HSM事件文件标识符指向该文件
        以非阻塞方式写入该队列 */
    llapi_hsm_event_fd = open(path, O_WRONLY | O_NONBLOCK);
```

### 4.3.2. 注销HSM事件队列
```c++
    /* 此API的目的是通过文件路径path注销HSM事件队列*/
    int llapi_hsm_unregister_event_fifo(const char *path);
```
此API的重要实现为：
```c++
    /* 关闭队列的读取与写入 */
    close(llapi_hsm_event_fd)
    // 取消与path的连接
    unlink(path);
    // 设置HSM事件队列的存在标志位为否
    created_hsm_event_fifo = false;
    // 设置HSM事件文件夹标识符为-1
    llapi_hsm_event_fd = -1;
```

---
## 4.4. HSM copytool

### 4.4.1. copytool结构

该结构体如下：

```c++
struct hsm_copytool_private {
	int				 magic;
	char				*mnt; // 代指Lustre的mount point
    struct kuc_hdr			*kuch; // 指向的是-> KUC信息头
    // 所以当前和未来的KUC信息必须包含在此头内
    // 目的是为了避免不得不去包含Lustre的libcfs头文件
	int				 mnt_fd; // mount所对接的文件
	int				 open_by_fid_fd;
    struct lustre_kernelcomm	*kuc; // 指向的是-> kernelcomm控制结构
    // 从用户态到内核态
    // 内核以存档的位图方式读取lk_data_count
};
```


### 4.4.2. copyaction结构
该结构体如下：

```c++
struct hsm_copyaction_private {
	__u32					 magic;
	__u32					 source_fd; // 源文件标识符
	__s32					 data_fd; // 数据标识符
	const struct hsm_copytool_private	*ct_priv; // 指向copytool的指针
	struct hsm_copy				 copy;
	lstat_t					 stat;
};
```

### 4.4.3. 注册copytool
具体API如下：

```c++
    /*  此API的目的是注册一个copytool
    若返回值为0说明成功注册，返回值为负则出错 */
    int llapi_hsm_copytool_register(struct hsm_copytool_private **priv,
    const char *mnt, int archive_count, int *archives, int rfd_flags);
    /*API调用方给出一个hsm_copytool_private结构的二重指针priv,
    一个Lustre的mount point的路径mnt,
    一个有效归档总数archive_count,
    一个copytool负责的归档集号码archives,
    和适用于通道读取文件的标志rfd_flags */
```

### 4.4.4. 注销copytool
具体API如下：

```c++
    /*  此API的目的是注册一个copytool
    若返回值为0说明成功注册，返回值为负则出错 */
    int llapi_hsm_copytool_unregister(struct hsm_copytool_private **priv);
    // 注销hsm_copytool_private结构的二重指针priv所指向的copytool
```

### 4.4.5. 获取copytool的文件标识符
具体API如下：
```c++
    /*  此API的目的是获取copytool的文件标识符*/
    int llapi_hsm_copytool_get_fd(struct hsm_copytool_private *ct);
    /*  输入一个不透明的 hsm_copytool_private结构的指针ct
        返回ct->kuc->rfd, 即返回kuc的读取文件标识符 */
```

## 4.5. HSM活动列表

### 4.5.1. 接收活动列表

```c++
    /*  
    此API的目的是让copytool接收活动列表
    若返回值为0证明执行成功，返回负数则说明接收数据失败 
    */
    int llapi_hsm_copytool_recv(struct hsm_copytool_private *priv,
        struct hsm_action_list **hal, int *msgsize);
    /* 
    输入当前的copytool--->ct， 和一个活动表的指针--->hal，
    以及一个信息的bytes数量---> msgsize.
    按照bytes数量msgsize读取hal指向的活动表
    并将其传给priv指向的copytool
    */
```

### 4.5.2. 开始执行hsm活动表
```c++
    /*  
    此API的目的是让copytool开始执行活动列表
    此API必须在开始执行请求前，被copytool所调用
    若copytool仅仅想报一个错，则跳过此程序，跳转到llapi_hsm_action_end()
    若返回值为0证明执行成功，返回负数则说明执行失败 
    */
    int llapi_hsm_action_begin(struct hsm_copyaction_private **phcp,
        const struct hsm_copytool_private *ct,const struct hsm_action_item *hai,
        int restore_mdt_index, int restore_open_flags,bool is_error);
    /* 
    通过hcp的选择，该操作跳到llapi_hsm_action_progress() 和 llapi_hsm_action_end()
    ct指向copytool注册所需的hsm私有结构
    hai描述当前请求
    restore_mdt_index: 用于创建易变文件的MDT index，使用-1为默认值
    restore_open_flags: 易变文件创建模式
    is_error: 此次调用是否是仅仅为了返回一个错误
    */
```


### 4.5.3. 终止HSM活动表
具体API如下：

```c++
    /*  
    此API的目的是终止一个HSM活动进程
    该API只有在copytool终止处理所有请求后，才可被调用
    若返回值为0证明成功终止HSM活动进程，返回负数则说明终止进程失败 
    */
    int llapi_hsm_action_end(struct hsm_copyaction_private **phcp,
        const struct hsm_extent *he, int hp_flags, int errval);
    /*  
    he： 被复制数据最终的范围
    errval: 操作的状态码
    hp_flags: 终止状态的标志位
    */
```

### 4.5.4. 通知HSM进程
具体API如下：

```c++
    /* 
    此API的目的是在处理一个HSM行为时，通知一个进程
    若返回值为0证明成功，返回负数则说明失败 
    */
    int llapi_hsm_action_progress(struct hsm_copyaction_private *hcp,
        const struct hsm_extent *he, __u64 total, int hp_flags);
    /* 
    hcp 指向的是当前的复制行为
    he： 被复制数据的范围
    total: 所需全部复制数据
    hp_flags: HSM进程标志位
    */
```

### 4.5.5. 获取被复制对象的文件标识符
具体API如下：
```c++
    /*
    此API的目的是获取被复制对象的文件标识符
    该文件标识符是通过传入的指针fid返回
    返回0如果操作成功，返回错误码如果此行为不是一个复制行为
    */
    int llapi_hsm_action_get_dfid(const struct hsm_copyaction_private *hcp, struct lu_fid *fid);
    /*
    hcp 为复制行为的指针
    fid为传入的指针，用于返回被复制对象的文件标识符
    */
```
该API会创建一个HSM活动项目：
```c++
    /* 该结构是为了描述copytool项目行为的 */
    struct hsm_action_item {
        __u32      hai_len;     /* 结构的合理大小 */
        __u32      hai_action;  /* 用已知的大小构建的copytool行为 */
        struct lu_fid hai_fid;     /* 需要去操控的Lustre文件标识符 */
        struct lu_fid hai_dfid;    /* 用于获取数据的文件标识符*/
        struct hsm_extent hai_extent;  /* 需要操控的字节范围 */
        __u64      hai_cookie;  /* 协调器的行为cookie */
        __u64      hai_gid;     /* 组锁ID */
        char       hai_data[0]; /* 可变长度 */
    } __attribute__((packed));
```

```c++
    // 若是以下两个条件不满足，则报错
    if (hcp->magic != CP_PRIV_MAGIC)
        return -EINVAL;
    if (hai->hai_action != HSMA_RESTORE && hai->hai_action != HSMA_ARCHIVE)
        return -EINVAL;
    // 上面创建的copytool项目行为的dfid, 就是待返回的dfid
    *fid = hai->hai_dfid;
```

### 4.5.6. 获取文件描述符
具体API如下：

```c++
    /*
    此API的目的是获取文件描述符用于复制数据
    该文件描述符是通过传入的copyaction指针返回
    返回0如果操作成功，返回负的错误码如果操作失败
    */
    int llapi_hsm_action_get_fd(const struct hsm_copyaction_private *hcp);
```

该API会创建一个HSM活动项目：

```c++
    /* 该结构是为了描述copytool项目行为的 */
    struct hsm_action_item {
        __u32      hai_len;     /* 结构的合理大小 */
        __u32      hai_action;  /* 用已知的大小构建的copytool行为 */
        struct lu_fid hai_fid;     /* 需要去操控的Lustre文件标识符 */
        struct lu_fid hai_dfid;    /* 用于获取数据的文件标识符*/
        struct hsm_extent hai_extent;  /* 需要操控的字节范围 */
        __u64      hai_cookie;  /* 协调器的行为cookie */
        __u64      hai_gid;     /* 组锁ID */
        char       hai_data[0]; /* 可变长度 */
    } __attribute__((packed));
```

```c++
    // 如果magic不对，报错
	if (hcp->magic != CP_PRIV_MAGIC)
		return -EINVAL;
    // 如果活动为Archive或Restore并且fd<0，则报错
	if (hai->hai_action == HSMA_ARCHIVE) {
		fd = dup(hcp->source_fd);
		return fd < 0 ? -errno : fd;
	} else if (hai->hai_action == HSMA_RESTORE) {
		fd = dup(hcp->data_fd);
		return fd < 0 ? -errno : fd;
	} else {
		return -EINVAL;
	}
```

### 4.5.7. 引入已存在的HSM归档文件
具体API如下：

```c++
int llapi_hsm_import(
    const char *dst,    // dst指向Lustre目的地路径
    int archive,        // archive指向档案数
    const struct stat *st, // st包含文件所属关系，置换等
    // stripe:条目选项，当前默认忽视。
    unsigned long long stripe_size, 
    int stripe_offset,
    int stripe_count, 
    int stripe_pattern, 
    char *pool_name,
    struct lu_fid *newfid   // newfid: 输入新的Lustre文件的fid，作为输出
);
```

调用完此API后，调用者必须用获得的新文件标识符去获取文件


## 4.6. HSM用户界面
### 4.6.1. 分配HSM用户请求
具体API如下：

```c++
    /*
    此API的目的是用特定的特征去分配HSM用户请求
    如果返回一个被分配的结构则成功，否则失败。
    */
    struct hsm_user_request *llapi_hsm_user_request_alloc(int itemcount, int data_len);
```

此结构应该能使用free()函数进行释放。

### 4.6.2. 发送HSM请求
具体API如下：

```c++
    /*
    此API的目的是用特定的特征去分配HSM用户请求
    返回0表示发送成功，负数表示发送失败
    */
    int llapi_hsm_request(
        const char *path,   // 待操作文件的全路径
        const struct hsm_user_request *request   // 需要向Lustre发送的用户请求
    );
```

### 4.6.3. 返回当前HSM请求

```c++
    /*
    此API的目的是通过被path所指的文件，返回其当前的HSM请求
    返回0表示发送成功，负数表示发送失败
    */
    int llapi_hsm_current_action(
        const char *path, // 通过path获取所指的文件
        struct hsm_current_action *hca // 被调用者所分配，指向当前文件行为
    );
```


--- 
# 5. Lemur总体架构分析
Lemur是一种遵循lustre HSM架构的跨层冷热数据实现方案。lustre提供一套针对HSM相关的用户调用接口，由用户自定义实现满足业务需求的冷热数据迁移方案。Lemur与Lustre的关系图如下： 

![Lemur总体架构](./img/lemur.png ''Lemur总体架构'')

上图中，对于每个MDT可以开启相应的HSM功能。开启lustre的HSM功能时，在会相应的MDT上启动一个Coordinator的守护进程，用于处理和派发HSM命令。Lemur在选定的lustre客户端上通过调用lustre的开发接口向所有开启HSM功能的MDT上注册一个或者多个Proxy。Proxy用于轮训来自MDT的HSM事件并接收其命令，之后将该命令按照一定的策略派发到相应的Copytool服务中。Copytool与Proxy通过gRPC连接，其所为gRPC的客户端存在，负责执行相应的HSM命令。 

每个MDT可以注册多个Proxy，MDT会按照一定的最佳选择策略将HSM命令派发到某个Proxy。每个Proxy可以连接多个Copytool，Proxy将HSM命令按照Archive ID与Copytool映射表，将HSM命令再次派发到对应的Copytool进行执行。每个Proxy最多可注册32个不同Archive ID的Copytool。Copytool执行具体的HSM命令时可以向Proxy返回其执行进度，以方便Proxy确认命令是否按时按需执行。 

每个Copytool同样运行在一个lustre客户端上，其包括Plugin和DataMover两个主要模块。Plugin是对DataMover集合的一种抽象的表示，每个DataMover作为PluginClient由Plugin模块进行管理，且DataMover可以按照pluginClient的接口由用户进行自定义。当前Lemur支持posix-to-posix的DataMover以及posix-to-s3的DataMover。顾名思义，不同的DataMover应对不同的HSM后端。posix DataMover可以将冷数据存储到支持posix接口的冷数据存储中，而s3 DataMover可以将冷数据存储到支持s3接口的冷数据存储中。 

除此之外，lustre的HSM架构中，还有一个PolicyEngine模块是必不可少的。PolicyEngine是运行在一个或者多个lustre客户端上的HSM策略服务，其通过lustre MDT导出的ChangeLog，收集存储在lustre中的元数据以及用户操作日志，并将这些日志存储到后端大数据分析平台中。这些数据会被用于分析当前lustre存储系统的状态和数据的分布及活动状况，通过用户定义HSM策略，执行相应的HSM命令。比如，可以将超过10天未被访问的某些类型的文件下沉到冷数据存储中；可以根据当前业务的需求按照某种数据预期策略提前将需要的数据预取到lustre中等操作。 





# 6. Proxy模块分析

Proxy充当HSM命令派发与Copytool管理的功能，其主要架构如下:

![Proxy模块](img/proxy.png ''Proxy模块'')

Proxy启动时首先加载配置文件，之后根据配置文件挂载lustre客户端，并创建以及注册lustre HSM的客户端。该客户端同时 又作为gRPC的服务端，管理DataMover客户端以及HSM命令的派发。创建HSM客户端之后，proxy会启动HSM poll线程以同步的方式接收来自lustre的HSM命令，并将这些命令放入一个全局队列中。之后启动一个或多个handler线程用于从全部队列中取出HSM命令，并放入对应的客户端的sync channel中。HSM同时会启动一个Plugin监控线程以及一个gRPC服务线程。Plugin监控线程用于周期性的确认每个DataMover实例是否工作正常，以及服务不可用时的处理办法。gRPC服务线程用于为gRPC客户端提供服务。gRPC服务启动时，会向外部导出三个服务接口： 

## 6.1. RPC Register
该RPC接口由DataMover调用，用于向Proxy注册一个DataMover实例。每个注册的DataMover实例都会按照Archive ID为索引的全部map中。每个DataMover实例在一个Proxy中必须唯一。通过Archive ID可以索引到对应的gRPC客户端handle，服务端可以通过该handle与gRPC客户端进行通信。另外，通过Archive ID也可以获取对应到该客户端的sync channel，每个派发到该客户端的HSM命令都会放到该channel中。channel中的HSM命令并不会由Proxy主动推送到客户端中，必须由客户端主动调用rpc GetAction获取对应的HSM命令。 

## 6.2. RPC GetAction

客户端主动调用该rpc，用户同步的获取属于该客户端的HSM命令。如果不存在发往该客户端的命令，则该客户端会被阻塞。如果Proxy端存在发往该客户端的HSM命令，则Proxy会调用rpc Send将sync channel的HSM命令取出并发往对应的客户端。 

## 6.3. RPC Status Stream

该rpc调用用于DataMover实例向Proxy汇报数据搬运的进度，Proxy接收 到进度信息时会进度进一步封装并发回到CDT服务中，CDT服务根据进度信息可以间接确认Proxy的活动状态。 

Global endpoints map是Proxy中重要的数据结构。客户端主动调用Register时Proxy会在该map中登记客户端对应的信息。Plugin monitor监测到客户端DataMover出现不可恢复异常时，会将对应的客户端项从该表中删除。 



# 7. Plugin模块分析

Plugin用于管理一个或者多个DataMover，其提供一种通用的DataMover操作接口，具体的接口的实现则由用户根据不同的存储后端实现不同的DataMover。Plugin模块的主要架构如下： 

![Plugin模块](img/plugin.png ''Plugin模块'')

Plugin模块定义可一种通用的DataMover操作框架，用户仅需要实现Plugin中定义的archive，restore以及remove操作。在Plugin中，每一个DataMover对应一个dmClient。每个dmClient对应一个archive ID，负责从Proxy中获取属于自己的HSM命令。dmClient在启动时会创建一个polling线程，该线程不断的调用GetAction从Proxy获取一个HSM命令，之后将该命令放到一个同步的全局actions channel中。dmClient启动一个或者多个action handle线程，每个线程从全局的同步channel中不断的读取HSM命令。读取到HSM命令时将调用dmio定义的接口进行数据拷贝，同时该线程不断的将数据拷贝的进度发送到一个同步的全局status channel中。status线程不断的从status channel获取进度数据，并通过调用StatusStream将进度数据不断的发送到Proxy中。 


# 8. DataMover模块

DataMover当前需要实现下属三种接口:

```c++
    // 将数据从lustre中下沉到冷存储中 
    func (m *Mover) Archive(action dmplugin.Action) error  
    // 将数据从冷存储中拷贝到lustre中 
    func (m *Mover) Restore(action dmplugin.Action) error  
    // 将数据中冷数据中删除 
    func (m *Mover) Remove(action dmplugin.Action) error  
```

上述函数的参数为Action接口。在Lemur中，Action的实现者是dmAction，其定义如下： 
```c++
    dmAction struct {
        // 操作执行进度信息的读写channel
        status chan *pb.ActionStatus    
        // 由protobuf定义的ActionItem的指针
        item *pb.ActionItem                 
        // Action实际需要拷贝数据的长度
        actualLength *int64                 
        // lustre xattr属性中uuid属性
        uuid string                                 
        // lustre xattr属性中hash属性
        hash []byte                                 
        // lustre xattr属性中url属性
        url string                                 
    } 
```

其中uuid，url以及hash可以用于唯一标识一个文件的id，或者存储一些用户希望的数据。比如hash值可以用于确认文件内容是否相同，在数据下沉时可以做数据去重。 

Lemur当前实现了posix DataMover以及s3 DataMover。 
