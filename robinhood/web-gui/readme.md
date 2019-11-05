# 1. 审计页面的调用数据库模块
本文主要是在审计页面中, 对于调用数据库模块做一个解析.

## 1.1. 连接方式

PHP连接Mysql实际上有MySQLi和PDO两种方式, 在此页面中, 我们使用PDO的连接方式, 示例如下:

```php
$conn = new PDO("mysql:host=$servername;", $username, $password);
```


## 1.2. 页面与数据库的连接

连接实际发生在配置php文件中, 而在服务器上配置页面时, 也是设置该配置文件中的参数. 

由于我们可以在配置中设置多个服务器中的多个数据库, 所以在配置中, 使用一个数据库的数组来实现. 

在页面配置中, 默认数据库main设置为当前服务器的Mysql数据库. 

而在此前端中, 实际连接与上述PDO连接类似:

```php
foreach ($DBA as $k=>$v) {

    $db[$k] = new PDO($DBA[$k]["DB_TYPE"].":host=".$DBA[$k]["DB_HOST"].";dbname=".$DBA[$k]["DB_NAME"], $DBA[$k]["DB_USER"], $DBA[$k]["DB_PASSWD"]);

    $db[$k]->exec("USE ".$DBA[$k]["DB_NAME"].";");

```

可以看出, 实际中数据库数组中储存的是各个数据库的PDO连接指针.

而在创建完每个指针后, 也会设置PDO数据库指针指向的配置中的数据库.













# 2. 数据库调用函数


## 2.1. 获取特定类型的数据库数组

#### 函数名为:  getDB

输入类型的名字

在全局变量DBA中, 如果类型名在对应数据库的DB_USAGE数组中, 将该数据库名字加入到返回数组中

返回该数组


## 2.2. 数据库初始化

#### 函数名为: pre_init()

利用getDB函数, 读取数据库数组的第一个数据库, 并做如下的初始化处理:

```php
$result = $db[$confdb]->query("
    SELECT * FROM information_schema.columns 
    WHERE (table_name = '$table') 
    AND TABLE_SCHEMA = '".$DBA[$confdb]["DB_NAME"]."';");
```


## 2.3. 获取ACCT表的列名

#### 函数名为: get_acct_columns()

从当前数据库的ACCT_STAT表中获取数据列, 处理完成后清空或者转化它们.

如果当前数据库不在数据库数组中, 返回全局变量final.

实际中使用的sql语句如下:
```sql
select column_name 
    from information_schema.columns 
    where table_name = 'ACCT_STAT' 
    and TABLE_SCHEMA = '".$DBA[$CURRENT_DB]["DB_NAME"]."';
```

返回的当前数据库的ACCT_STAT表的列名称.


## 2.4. SQL过滤器

### 2.4.1. 函数名为: build_sql_access($part)

一个简单的sql过滤器, 只允许特定用户或群组使用sql语句, 防止页面sql注入攻击.

将part中的用户名和组名设置到过滤器中, 过滤掉非这些用户与群组的sql访问.

### 2.4.2. 函数名为: get_filter_from_list($datalist, $term)

从数据队列中得到过滤器

### 2.4.3. 函数名为: build_filter($args, $filter, $self='$SELF')

REST传入args参数为key/value数组, self为只展示用户信息,

遍历输入的filter过滤器, 调用上面的get_filter_from_list函数. 

如果filter其中的过滤器存在于args参数中, 如min, max, where, having等, 就加入过滤器数组.

该函数创建SQL请求, 返回sql请求数组和过滤器数组.




### 2.4.4. 函数名为: build_advanced_filter($args, $access = '$SELF', $table, $join = false)

从REST传入参数中创建SQL请求,  但是有用户限制与权限表, 并且可以实现左联另一个表, 参考上面build_filter中的实现, 返回sql请求数组和过滤器数组

该函数实现的查询语句主体如下:

```sql
SELECT column_name,column_type,table_name
    FROM information_schema.columns 
    WHERE ($ttable) 
    AND TABLE_SCHEMA = '".$DBA[$CURRENT_DB]["DB_NAME"]."';

根据不同的Rest语句, 可以实现的操作有:

count 统计出现次数  max 最大值  min 最小值

avg 平均值  sum 统计总数    concat  连接

remove  移除    filter  过滤    nfilter 反向过滤

equal   相等    less    小于    bigger  大于

soundslike 类似 asc 升序排列    desc 将序排列

Left Join 左联另一个表    group by 以某一列分组    limit 输出范围限制

```


# 3. 审计页面交互

根据请求的不同, 审计页面有五种交互方式.

原生数据展示, 图形展示, 输出json都用这五种交互方式向数据库服务器发送sql请求:

## 3.1. 变量请求

根据传入的VARS变量, 直接使用下列语句查询当前数据库:

```sql
SELECT * from VARS;
```

## 3.2. acct表请求

调用build_advanced_filter函数, 查看当前请求能否通过***ACCT_STAT表***构成的高级SQL过滤器, 可以防止SQL注入.

只有存在与ACCT_STAT表中的数据, 才可以得到展示

## 3.3. files请求

调用build_advanced_filter函数, 查看当前请求能否通过***NAMES表左联ENTRIES表***构成的高级SQL过滤器, 可以防止SQL注入.

## 3.4. entries请求

调用build_advanced_filter函数, 查看当前请求能否通过***ENTRIES表***构成的高级SQL过滤器, 可以防止SQL注入.


## 3.5. names请求

调用build_advanced_filter函数, 查看当前请求能否通过***NAMES表***构成的高级SQL过滤器, 可以防止SQL注入.


# 4. 页面图形展示

根据请求的不同, 页面展示图形有如下的不同:

## 4.1. 用户名请求

无展示, 页面为空

## 4.2. 组名请求

以组名请求中的参数, 调用build_filter构建sql过滤器, 有用户名, 组名, 最大大小, 最小大小, 共四个过滤器.

通过过滤器的请求, 会调用如下的sql语句:

```sql
SELECT $content_requested, 
    SUM(size) AS ssize, 
    SUM(count) AS scount 
    FROM ACCT_STAT $sqlfilter 
    GROUP BY $content_requested $havingfilter;
```

## 4.3. Sizes请求

以Sizes请求中的参数args, 调用build_filter构建sql过滤器, 有用户名, 组名两个过滤器.

通过过滤器的请求,会将大小分为空文件, 1Byte, 32B, 1KB, 32KB, 1MB, 32KB, 1GB, 32GB, 1TB 等10个类别.

```sql
SELECT $select_str FROM ACCT_STAT $sqlfilter;
```

然后将数据库中的文件, 以大小分类到该十类下, 展示出该图形.

## 4.4. Files请求

以Files请求中的参数args, 调用build_filter构建sql过滤器, 有文件名, 用户名, 组名, 偏移量, 最大大小, 最小大小, 共六个过滤器.

通过过滤器的请求, 会调用以下的sql语句, 

```sql
SELECT uid, gid, size, blocks, name, type, creation_time, last_access, last_mod "."
    FROM ENTRIES 
    LEFT JOIN NAMES 
    ON ENTRIES.id = NAMES.id $sqlfilter 
    LIMIT $MAX_ROWS OFFSET $offset;
```

得出的文件元数据, 以图形的形式展示出来.


## 4.5. 默认请求

不满足上述的四种请求的, 以参数args, 调用build_filter构建sql过滤器, 有文件名, 用户名, 组名, 共三种过滤器.

通过过滤器的请求, 调用以下的sql语句:

```sql
SELECT $content_requested AS sstatus, 
    SUM(size) AS ssize, 
    SUM(count) AS scount 
    FROM ACCT_STAT $sqlfilter 
    GROUP BY $content_requested;
```

得出的文件元数据, 以图形的形式展示出来.





