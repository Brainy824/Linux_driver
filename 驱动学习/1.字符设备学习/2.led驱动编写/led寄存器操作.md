[toc]

## 总结

#### 1、地址映射

1、Linux驱动开发也可以操作寄存器，Linux不能直接对寄存器物理地址进行读写操作，比如寄存器A物理地址为0x01010101。裸机的时候可以直接对0x01010101这个物理地址进行操作，但是Linux下不行。因为Linux会使能MMU。

在Linux里面操作的都是虚拟地址，所以需要先得到0x01010101这个物理地址对应的虚拟地址。

获得物理地址对应的虚拟地址使用ioremap（）函数。

第一个参数就是物理地址起始大小，第二个参数就是要转化的字节数量。0x01010101,开始10个地址进行转换。

va = ioremap(0x01010101,10);

led_exit()卸载驱动的时候；iounmap(va),释放地址映射



#### 2、LED灯字符设备驱动框架搭建

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>


#define LED_MAJOR   200
#define LED_NAME    "led"



static int led_open(struct inode *inode,struct file *filp)
{
    return 0;
}

static int led_release(struct inode *inode,struct file *filp)
{
    return 0;
}

static ssize_t led_write(struct file *filp,const char __user *buf,size_t count,loff_t *ppos)
{
    return 0;
}


/*ops 字符设备操作集合*/
static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .write = led_write,
    .open  = led_open,
    .release = led_release,
};

/*input 入口函数*/
static int __init led_init(void)
{
    /*1.register_chrdev*/
    int ret = 0;
    ret = register_chrdev(LED_MAJOR,LED_NAME,&led_fops);
    if (ret < 0) {
        printk("register chardev failed\r\n");
        return -EIO;
    }

    printk("led_init\r\n");
    return 0;
}
/*output 出口函数*/
static void __exit led_exit(void)
{

    unregister_chrdev(LED_MAJOR,LED_NAME);

    printk("led_exit\r\n");
}
module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liutao");
```





#### 3、驱动程序编写

###### 加载驱动前，一定要记得 depmod 解决它的依赖

每执行一次驱动就会关闭一次驱动

初始化在入口函数或者open函数里面都可以，自己选择

1、初始化时钟，io（gpio不一定是引脚，也有可能是io内存），注意一定要在地址映射后点亮，在取消映射前关闭。

```c

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/io.h>


#define LED_MAJOR   200
#define LED_NAME    "led"



static int led_open(struct inode *inode,struct file *filp)
{
    return 0;
}

static int led_release(struct inode *inode,struct file *filp)
{
    return 0;
}

static ssize_t led_write(struct file *filp,const char __user *buf,size_t count,loff_t *ppos)
{
    return 0;
}


/*ops*/
static const struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .write = led_write,
    .open  = led_open,
    .release = led_release,
};

/*input */
static int __init led_init(void)
{
    /*1.register_chrdev*/
    int ret = 0;
    ret = register_chrdev(LED_MAJOR,LED_NAME,&led_fops);
    if (ret < 0) {
        printk("register chardev failed\r\n");
        return -EIO;
    }

    printk("led_init\r\n");
    return 0;
}
/*output*/
static void __exit led_exit(void)
{

    unregister_chrdev(LED_MAJOR,LED_NAME);

    printk("led_exit\r\n");
}
module_init(led_init);
module_exit(led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("liutao");
```



#### 4、应用程序编写

都是简单的open -》 read、write -》close

内核里面每执行一次驱动就会关闭一次驱动

,还要记得加依赖depmod

记得一定要将接收到的数据转换为字符型，因为内核里面是字符型，所以要转换atoi();

记得驱动加载后要记得mknod /dev/led c 200 0（创建字符设备节点，否则APP无法运行）

![image-20230722164629887](C:\Users\DELL\Desktop\linux_driver\Linux_driver\驱动学习\字符设备学习\2.led驱动编写\加载字符设备节点)

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

/*
 *argc:
 *argv[]:
 * ./ledAPP <filename> <0:1>
 * ./ledAPP /dev/led 0
 * ./ledAPP /dev/led 1
 * 
*/
#define LEDOFF 0
#define LED0N  1
int main(int argc,char *argv[])
{
    int fd,ret;
    char *filename;
    unsigned char databuf[1];

    if(argc != 3) {
        printf("error usage\r\n");
    }

    filename = argv[1];
    fd = open(filename,O_RDWR);
    if(fd < 0) {
        printf("file %s open failed\r\n",filename);
        return -1;
    }
    databuf[0] = atoi(argv[2]);/*become number*/
    ret = write(fd,databuf,sizeof(databuf));
    if(ret < 0) {
        printf("LED control failed\r\n");
        close(fd);
        return -1;
    }
    close(fd);
    return 0;
}
```



#### 5、注意开发板的地址别和同网段的其他设备ip冲突，不然会导致开发板启动时不可以在Ubuntu下载镜像的去启动开发板（实际上，如果你是网线连接电脑到开发板，一般不会有这个问题，因为桥接的）

1、Uboot下载系统设备，以前可以下载成功（也就是成功启动开发板），突然不能下载怎么解决？

首先，保证整个网段内开发板的ip地址和ubuntu的ip地址是唯一的，测试那个ip地址有冲突，比如Ubuntu的192.168.1.25有没有被其他设备占用，如果有占用就改一个没被占用的IP地址。

2、bootargs和bootcmd也得修改主机地址（Ubuntu地址）

