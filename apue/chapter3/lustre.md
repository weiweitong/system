[toc]

## 1 HSM支持的操作

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

| state | explanation |
| :--: | :---: |
| Non-archive | 标识为该状态的文件将不会被archive |
| Non-release | 标识为该状态的文件将不会被release  |
| Dirty | 存储在lustre中的文件以被更改，需要执行archive操作以便更新HSM中的副本 |
| Lost | 该文件已被archive，但是存储的HSM中的副本已丢失 |
| Exist  | 文件的数据拷贝在HSM中已经存在 |
| Rleased | 文件的LOV信息以及LOV对象已从lustre中移除 |
| Archived    | 文件数据已整个从lustre中拷贝至HSM中  |




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

## 2 Lustre的HSM接口

lustre向外部导出C语言的API接口，主要的接口定义如下:

### 2.1 获取HSM状态

#### 2.1.1 HSM状态

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

#### 2.1.2 通过文件标识符获取hsm状态

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

#### 2.1.3 通过文件路径获取HSM状态

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

### 2.2 设置HSM状态

#### 2.2.1 通过文件标识符设置hsm状态

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

#### 2.2.2 通过文件路径设置hsm状态

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

#### 2.2.3 HSM日志报错

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

### 2.3 HSM事件队列

#### 2.3.1 注册HSM事件队列

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

#### 2.3.2 注销HSM事件队列

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

### 2.4 HSM copytool

#### 2.4.1 copytool结构

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

#### 2.4.2 copyaction结构

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

#### 2.4.3 注册copytool

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

#### 2.4.4 注销copytool

具体API如下：

```c++
    /*  此API的目的是注册一个copytool
    若返回值为0说明成功注册，返回值为负则出错 */
    int llapi_hsm_copytool_unregister(struct hsm_copytool_private **priv);
    // 注销hsm_copytool_private结构的二重指针priv所指向的copytool
```

#### 2.4.5 获取copytool的文件标识符

具体API如下：
```c++
    /*  此API的目的是获取copytool的文件标识符*/
    int llapi_hsm_copytool_get_fd(struct hsm_copytool_private *ct);
    /*  输入一个不透明的 hsm_copytool_private结构的指针ct
        返回ct->kuc->rfd, 即返回kuc的读取文件标识符 */
```

### 2.5 HSM活动列表

#### 2.5.1 接收活动列表

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

#### 2.5.2 开始执行hsm活动表

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

#### 2.5.3 终止HSM活动表

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

#### 2.5.4 通知HSM进程

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

#### 2.5.5 获取被复制对象的文件标识符

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

#### 2.5.6 获取文件描述符

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

#### 2.5.7 引入已存在的HSM归档文件

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

### 2.6 HSM用户界面

#### 2.6.1 分配HSM用户请求

具体API如下：

```c++
    /*
    此API的目的是用特定的特征去分配HSM用户请求
    如果返回一个被分配的结构则成功，否则失败。
    */
    struct hsm_user_request *llapi_hsm_user_request_alloc(int itemcount, int data_len);
```

此结构应该能使用free()函数进行释放。

#### 2.6.2 发送HSM请求

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

#### 2.6.3 返回当前HSM请求

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


