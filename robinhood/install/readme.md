
# 1. Robinhood安装命令

## 1.1. 安装依赖
sudo yum install -y git autogen rpm-build autoconf automake gcc libtool glib2-devel libattr-devel mariadb-devel mailx bison flex jemalloc jemalloc-devel


## 1.2. 安装robinhood

### 从github下载代码
sudo git clone https://github.com/cea-hpc/robinhood.git

### 进入目录
cd robinhood

### pipeline执行
sudo ./autogen.sh

### 构建RPM包
sudo ./configure

sudo make rpm

### 安装所有RPM包
cd rpms/RPMS/x86_64/

sudo yum -y localinstall robinhood-*

## 1.3. 安装MySQL或者MariaDB

DB server可运行在与robinhood server不同的节点上，但是为了减少延迟也可将DB server与robinhood安装在同一节点上。DB安装步骤略，安装成功后需打开3306端口。

sudo yum install mariadb-server -y


## 1.4. 创建robinhood database

mysqladmin create robinhood_db

mysql robinhood_db

mysql> create user robinhood;

mysql> GRANT USAGE ON robinhood_db.* TO 'robinhood'@'localhost';

mysql> GRANT ALL PRIVILEGES ON robinhood_db.* TO 'robinhood'@'localhost';

mysql> GRANT SUPER ON *.* TO 'robinhood'@'localhost';

mysql> FLUSH PRIVILEGES;


## 1.5. robinhood 配置

默认情况下robinhood配置文件路径为/etc/robinhood.d/*.conf

- 对单文件系统，可创建名为/etc/robinhood.d/<myfs>.conf的配置文件，<myfs>为随意指定的名称
- 对多文件系统，每个文件系统都要创建一个对应的配置文件，如/etc/robinhood.d/<myfs1>.conf，/etc/robinhood.d/<myfs2>.conf

vim /etc/robinhood.d/*.conf

robinhood RPM将配置模版安装在/etc/robinhood.d/templates/目录下，其中basic.conf为基础配置文件，可将其复制到/etc/robinhood.d/目录下并进行修改。其内容如下：

```bash
General {

    # 文件系统挂载点

    fs_path = "/path/to/fs";

    fs_type = fstype; # eg. lustre, ext4, xfs...

 }

 Log {

    # 日志文件，需确保具有读写权限

    log_file = "/var/log/robinhood/myfs.log";

    report_file = "/var/log/robinhood/myfs_actions.log";

    alert_file = "/var/log/robinhood/myfs_alerts.log";

 }

 ListManager {

    MySQL {

        # DB server的host

        server = db_host;

        # 在第4步中创建的robinhood database实例

        db = robinhood_test;

        # DB user

        user = robinhood;

        # DB的password存放文件，也可直接写password = 'pwd';

        password_file = /etc/robinhood.d/.dbpassword;

    }

 }

 # Lustre 2.x only

 # Lustre 2.x的ChangeLog功能，需先在MDS上注册ChangeLog reader，然后在下方填入MDT name和注册reader时返回的id；注意MDT name格式为MDT<id>

 ChangeLog {

    # if you have multiple MDT (DNE)

    # define one MDT block per MDT

    MDT {

        mdt_name = "MDT0000";

        reader_id = "cl1";

    }

 }
```

## 1.6. 初始扫描

若只有一个文件系统（只有一个配置文件），则可直接运行

robinhood --scan --once

若有多个文件系统（有多个配置文件），则扫描时需指定配置文件

robinhood -f <myfs> --scan --once

其中myfs为文件系统对应的配置文件名，也可以直接输入完整的配置文件路径如

robinhood -f /home/foo/test.conf --scan --once




## 1.3. 

General {
    fs_path = "/mnt/lustre/";
    # filesystem type, as displayed by 'mount' (e.g. ext4, xfs, lustre, ...)
    fs_type = lustre;
}

Log {
    log_file = "/var/log/robinhood.log";
    report_file = "/var/log/robinhood_actions.log";
    alert_file = "/var/log/robinhood_alerts.log";
}

ListManager {
    MySQL {
        server = localhost;
        db = robinhood_db;
        user = robinhood;
        password = '';
    }
}

# Lustre 2.x only
ChangeLog {
    MDT {
        # name of the first MDT
        mdt_name  = "MDT0000";

        # id of the persistent changelog reader
        # as returned by "lctl changelog_register" command
        reader_id = "cl1";
    }
}



## 2.1. Lustre命令

获取lustre挂载点

lfs df -h


## 2.2. Robinhood命令

显示扫描得到的大小，从1到1T以上的范围

rbh-report -f lustre -i --szprof



## 2.3. PHP命令


### 2.3.1. PHP命令
robinhood-webgui-3.1.5-1.x86_64

```bash
Package php-mysql-5.4.16-46.el7.x86_64 is obsoleted by php-mysqlnd-7.2.23-1.el7.remi.x86_64 which is already installed
--> Finished Dependency Resolution
Error: Package: robinhood-webgui-3.1.5-1.x86_64 (/robinhood-webgui-3.1.5-1.x86_64)
           Requires: php-mysql
           Available: php-mysql-5.4.16-46.el7.x86_64 (base)
               php-mysql = 5.4.16-46.el7
           Available: php-mysqlnd-5.4.16-46.el7.x86_64 (base)
               php-mysql = 5.4.16-46.el7
           Available: php-pecl-mysql-1.0.0-0.17.20160812git230a828.el7.remi.7.2.x86_64 (remi-php72)
               php-mysql = 1:1.0.0
           Installed: php-mysqlnd-7.2.23-1.el7.remi.x86_64 (@remi-php72)
               Not found
           Available: php-mysqlnd-7.2.22-1.el7.remi.x86_64 (remi-php72)
               Not found
```

需要的是php54

```bash
 cd /etc/yum.repos.d/  

rm -f remi-php72.repo

# 查找rpm包
rpm -qa | grep epel  

# 删除rpm包
rpm -e rpm.name

```


安装PHP
