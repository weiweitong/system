# 1. get info from server

```bash
# get linux info
uname -a

Linux host-10-5-9-241 3.10.0-957.el7.x86_64 #1 SMP Thu Nov 8 23:39:32 UTC 2018 x86_64 x86_64 x86_64 GNU/Linux


# get cpu info
cat /proc/cpuinfo |grep "model name" && cat /proc/cpuinfo |grep "pysical id"

model name	: Intel Xeon Processor (Skylake)
model name	: Intel Xeon Processor (Skylake)
model name	: Intel Xeon Processor (Skylake)
model name	: Intel Xeon Processor (Skylake)
model name	: Intel Xeon Processor (Skylake)
model name	: Intel Xeon Processor (Skylake)
model name	: Intel Xeon Processor (Skylake)
model name	: Intel Xeon Processor (Skylake)
physical id	: 0
physical id	: 1
physical id	: 2
physical id	: 3
physical id	: 4
physical id	: 5
physical id	: 6
physical id	: 7

# get memory info
cat /proc/meminfo |grep MemTotal

MemTotal:       32779820 kB

# get disk info
fdisk -l |grep Disk

Disk /dev/vda: 107.4 GB, 107374182400 bytes, 209715200 sectors
Disk label type: dos
Disk identifier: 0x000c477c
Disk /dev/vdb: 107.4 GB, 107374182400 bytes, 209715200 sectors
Disk /dev/mapper/centos-root: 96.6 GB, 96636764160 bytes, 188743680 sectors
Disk /dev/mapper/centos-swap: 8455 MB, 8455716864 bytes, 16515072 sectors
Disk /dev/mapper/centos-home: 1073 MB, 1073741824 bytes, 2097152 sectors

# get hostname
hostname

host-10-5-9-241

# list all the pci devices
lspci -tv

# list all the usb devices
lsusb -tv

# list all the kernel mod
lsmod

# list the environment
env

# check the used memory and the used swap
free -m

# check the used partition 
df -h

# check the size of appointed disk
du -sh
du /etc/ -sh

# get the total memory
grep MemTotal /proc/meminfo
cat /proc/meminfo |grep MemTotal

# get the available memory
grep MemFree /proc/meminfo
cat /proc/meminfo |grep MemFree

# check the running time, user count, load
uptime

12:16:29 up 51 days, 18:31,  1 user,  load average: 0.00, 0.01, 0.05

# check the system load and partition
cat /proc/loadavg

# check the mount partition
mount | column -t

# check all the partition
fdisk -l

# check all the swap disk 
swapon -s

# check the disk parameters
hdparm -i /dev/hda

# check the IDE device
dmesg |grep IDE

# check the attributes of the net API
ifconfig

# check the setting of the firewall
iptables -L

# check the route table
route -n

# check all the listen port
netstat -lntp

# check all the established connection
netstat -antp

# check all the network information process
netstat -s

# check all the process
ps -ef

```

# 2. search for file


```bash 
# find a file in 
find / -name "name"

```



# 3. 删除侧边栏

修改个人目录 ~/.config 下的 user-dirs.dirs 文件，将你不想要的都注释掉或者删除：

简单来说就是修改个人目录 ~/.config 下的 user-dirs.dirs 文件，将你不想要的都注释掉或者删除：

# This file is written by xdg-user-dirs-update
# If you want to change or add directories, just edit the line you're
# interested in. All local changes will be retained on the next run
# Format is XDG_xxx_DIR="$HOME/yyy", where yyy is a shell-escaped
# homedir-relative path, or XDG_xxx_DIR="/yyy", where /yyy is an
# absolute path. No other format is supported.
#
XDG_DESKTOP_DIR="$HOME/桌面"
XDG_DOWNLOAD_DIR="$HOME/下载"
#XDG_TEMPLATES_DIR="$HOME/模板"
#XDG_PUBLICSHARE_DIR="$HOME/公共的"
#XDG_DOCUMENTS_DIR="$HOME/文档"
#XDG_MUSIC_DIR="$HOME/音乐"
#XDG_PICTURES_DIR="$HOME/图片"
#XDG_VIDEOS_DIR="$HOME/视频"

再创建一个文件：
echo "enabled=false" > ~/.config/user-dirs.conf

重启看看，是不是清爽了。

