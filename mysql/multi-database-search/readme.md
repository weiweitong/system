# 跨库查询

# 1. 同一台服务器内跨库查询

同一台服务器内比较容易实现:

- 数据库db1中有表table1
- 数据库db2中有表table2
- 表db1.table1与表db2.table2有关联关系

跨库查询语句如下:

```sql
select 
    *
from 
    db1.table1 left join db2.table2
on
    db1.table1.user_id = db2.table2.uid
where
    1 limit 0, 10;


```

# 2. 不同服务器内跨库查询

Mysql仅支持同一台服务器上跨数据库的联表查询, 但并不支持跨服务器.

为了实现跨服务器查询, 有如下两种解决方式
- 程序层解决方案
- 数据库层解决方案
- 数据库引擎解决方案

## 2.1. 程序层解决方案

操作简单, 实现容易. 将查询命令分别对两个数据库单独查询, 将两个结果记录下来. 而后, 用程序语句来执行关联表关系的操作. 

如上面的查询语句:
```sql
# 可以先对服务器1中的数据1查询:
select * from db1.table1;
# 然后再对服务器2中的数据库2查询:
select * from db2.table2;
```
然后使用程序语句, 将两条信息中uid相等的实例,以数据库1左联的方式结合起来.

此种方法虽然容易理解, 易于实现, 但是效率低下, 如果查到的数据量很大时, 程序联合的速度较慢.


## 2.2. 数据库层解决方案

还是上述的跨库查询案例, 因为db1数据库中的table1左联db2中的table2, 所以可以:
- 先将db2中的table2导出, 导入到db1中.
- 此时,情况变成了1中的单服务器内跨库查询
- 单服务器跨库查询

优点: 较容易实现

缺点: 需要整表复制, 表内数据量大时仍然是瓶颈

## 2.3. 数据库引擎层解决方案


基于Mysql中的Federated引擎, 建表.

在mysql语句中, 使用下列语句查询是否支持Federated:
```sql
show engines;
```
如果没有Federated引擎, 但是support是No, 
```bash
cd /etc/
vim my.cnf
```
需要在my.cnf中末尾添加一行:
```bash
federated
```

重启Mysql
```bash
systemctl reatart mariadb
```

建表语句如下:
```sql
create table 'table_name' (...)
ENGINE = FEDERATED 
CONNECTION = 'mysql://[username]:[password]@[ip]:[port]/[db_name]/[table_name]'

```

通过Federated引擎创建的表只是在本地表定义文件, 数据文件则存在于远程数据库中, 通过这个引擎可以实现远程数据访问功能, 也即是这种建表方式只会在数据库1中创建一个db2的table2的表结构文件, table2的索引, 数据等文件还是在db2服务器上, 只是相当于数据库A中创建了table2的一个快捷方式.

用Federated注意的点有:
- 本地的表结构必须与远程的表结构完全一致
- 不支持事务
- 不支持表结构修改
  
优点: 实现简单, 快捷, 有完整的文档支持

缺点: 不支持事务, 不支持表结构修改
