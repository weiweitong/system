# 1. CentOS 7系统修改mariadb的数据目录

```bash

# dir size
df -h

# mysql default directory
/var/lib/mysql/

# find the mnt 
10.10.31.37
/mnt/mysql_data/mysql

cd /mnt/mysql_data
mkdir mysql
chown -R mysql:mysql mysql/
chmod 755 mysql/

systemctl stop mariadb

vim /etc/my.cnf
cat /etc/my.cnf
-----------------------------------
[mysqld]
# datadir=/var/lib/mysql
socket=/var/lib/mysql/mysql.sock
datadir=/mnt/mysql_data/mysql
#socket=/mnt/mysql_data/mysql/mysql.sock

# skip-grant-tables

# Disabling symbolic-links is recommended to prevent assorted security risks
symbolic-links=0
# Settings user and group are ignored when systemd is used.
# If you need to run mysqld under a different user or group,
# customize your systemd unit file for mariadb according to the
# instructions in http://fedoraproject.org/wiki/Systemd

[mysqld_safe]
log-error=/var/log/mariadb/mariadb.log
pid-file=/var/run/mariadb/mariadb.pid

#
# include all files from the config directory
#
!includedir /etc/my.cnf.d

-----------------------------------

# copy data file
cp -a /var/lib/mysql/* /mnt/mysql_data/mysql/


systemctl start mariadb
systemctl status mariadb

```

# 2. Can't connect to local MySQL server through socket '/tmp/mysql.sock'

## 2.1. docker

```bash
service mysql restart
```

## 2.2. centos

```bash
# build a tcp/ip connection
mysql -uroot -h 127.0.0.1 -p

chmod 777 /var/lib/mysql

service mysql restart
service mysqld restart

systemctl status mysqld.service

# if /var/run/mysqld is not exist
mkdir /var/run/mysqld
chmod 777 /var/run/mysqld/
service mysqld start

ls /vr/run/mysqld

# use the socket
mysql -u root -p -S /var/run/mysqld/mysqld.sock

# soft link
ln -s /var/run/mysqld/mysqld.sock /tmp/mysql.sock

```


# 3. Centos7 安装 PHP 5.4
```bash
## search the php rpm
rpm -qa |grep php

## delete the php

rpm -e name.rpm

yum list | grep php
```

# 4. Mysql操作

```sql
# get the row number
select
    count(*)
from
    table_name;

# get first 10 rows
select
    *
from
    table_name
limit 0,2;

# get the row number from 2 tables
select
    'customers' tablename,
    count(*) rows
from 
    customers
union
select
    'orders' tablename,
    count(*) rows
from orders;

```


# 5. Mysql user operation

```sql
# get into mysql
mysql -u root -p

# create user and set its password
create user 

```



