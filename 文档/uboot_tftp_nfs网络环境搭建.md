## 首先传文件到/home/local/linux/nfs

> 1、跟着第三期网络命令看
>
> tftpboot也一样

#### 2、移植后 需要添加的boot设置命令

```C
setenv ipaddr 192.168.1.50
setenv ethaddr b8:ae:1d:01:00:00
setenv gatewayip 192.168.1.1
setenv netmask 255.255.255.0
setenv serverip 192.168.1.253
saveenv
```



```C
setenv bootcmd 'tftp 80800000 zImage;tftp 83000000 imx6ull-alientek-emmc.dtb;bootz 80800000 - 83000000'
    
setenv bootargs 'console=ttymxc0,115200 rw nfsroot=192.168.25:/home/local/linux/nfs/rootfs ip=192.168.1.50:192.168.1.25:192.168.1.1:255.255.255.0::eth0:off'
    
    
lcd显示的
setenv bootargs 'console=tty1 console=ttymxc0,115200 root=/dev/nfs rw nfsroot=192.168.1.25:
/home/local/linux/nfs/rootfs ip=192.168.1.50:192.168.1.25:192.168.1.1:255.255.255.0::eth0:
off'
    
saveenv
```

