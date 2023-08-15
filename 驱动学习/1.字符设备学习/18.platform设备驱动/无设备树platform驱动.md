[toc]

## 总结（无设备树下编写驱动,应用程序与前面是一样的）

#### 两部分（点灯为例子）（创建设备程序，创建驱动程序）（一般是创建好后，驱动不用改，只改设备，属性能匹配上就好）

1、编写向platform总线注册设备。编写驱动需要寄存器地址信息，地址信息使用设备信息，定义在platform_device里面，因此需要在驱动里面获取设备中的信息，或者叫资源。

##### 1、platform_device(设备)

1、可以在开发板下查看到

```C
cd /sys/bus/platform/
ls
```

![图片](C:\Users\DELL\Desktop\linux_driver\Linux_driver\驱动学习\字符设备学习\18.platform设备驱动\设备信息.png)

2、完整的设备程序

```C
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>



/*寄存器物理地址*/
#define CCM_CCGR1_BASE                  (0x020c406c)
#define SW_MUX_GPIO1_IO03_BASE          (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE          (0x020E02F4)
#define GPIO1_DR_BASE                   (0x0209C000)
#define GPIO1_GDIR_BASE                 (0x0209C004)

#define REGISTER_LENGTH                 4

static void leddevice_release(struct device *dev)
{
        printk("leddevice release\r\n");
}

/*resource结构体*/
static struct resource led_resources[] = {
        [0] = {
                .start  = CCM_CCGR1_BASE,
                .end    = CCM_CCGR1_BASE + REGISTER_LENGTH - 1,
                .flags  = IORESOURCE_MEM,
        },

        [1] = {
                .start  = SW_MUX_GPIO1_IO03_BASE,
                .end    = SW_MUX_GPIO1_IO03_BASE + REGISTER_LENGTH - 1,
                .flags  = IORESOURCE_MEM,
        },
        
        [2] = {
                .start  = SW_PAD_GPIO1_IO03_BASE,
                .end    = SW_PAD_GPIO1_IO03_BASE + REGISTER_LENGTH - 1,
                .flags  = IORESOURCE_MEM,
        },
        
        [3] = {
                .start  = GPIO1_DR_BASE,
                .end    = GPIO1_DR_BASE + REGISTER_LENGTH - 1,
                .flags  = IORESOURCE_MEM,
        },
        
        [4] = {
                .start  = GPIO1_GDIR_BASE,
                .end    = GPIO1_GDIR_BASE + REGISTER_LENGTH - 1,
                .flags  = IORESOURCE_MEM,
        },
};

/*注册的结构体*/
static struct platform_device leddevice = {
        .name           = "imx6ull-led",/*设备写的什么名字，驱动也要些什么名字*/
        .id             = -1,/*为-1表示次设备无ID*/
        .dev            = {
                .release  = leddevice_release,/*有什么资源释放，可以在这里去释放*/
        },

        /*跟资源有关的*/
        .num_resources = ARRAY_SIZE(led_resources),
        .resource = led_resources,

};

/*设备加载*/
static int __init leddevice_init(void)
{
        int ret = 0;
        printk("leddevice_init\r\n");

        /*注册platform设备*/
        ret = platform_device_register(&leddevice);/*里面是指针，所有要取指*/

        return ret;
}

/*设备卸载*/
static void __exit leddevice_exit(void)
{
        printk("leddevice_exit\r\n");
        /*卸载platform设备*/
        platform_device_unregister(&leddevice);
}


module_init(leddevice_init);
module_exit(leddevice_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("liutao");
```

##### 2、platform_driver(驱动)

```C
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/irq.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fs.h>
#include <linux/fcntl.h>
#include <linux/platform_device.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>



#define PLATFORM_NAME "platform"
#define PLATFORM_COUNT 1

/*address virtual pointer*/
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

#define LEDOFF 0
#define LEDON 1

/*device object*/
struct newchrled_dev{
    struct cdev cdev; /*zifushebei*/
    dev_t devid; /*device number*/
    struct class *class;
    struct device *device;
    int major;
    int minor;
};
struct newchrled_dev newchrled; /*led device*/

/*led is open/close*/
static void led_switch(u8 sta)
{
    u32 val = 0;

    if(sta == LEDON) {
        val = readl(GPIO1_DR);
        val &= ~(1 << 3); /*led is open while GPIO1_DR is 0*/
        writel(val,GPIO1_DR);
    } else if(sta == LEDOFF) {
        val = readl(GPIO1_DR);
        val |= (1 << 3); /*led is open while GPIO1_DR is 0*/
        writel(val,GPIO1_DR);
    }
}

static int newchrled_open(struct inode *inode,struct file *filp)
{
    filp->private_data = &newchrled;
    return 0;
}

static int newchrled_release(struct inode *inode,struct file *filp)
{
    //struct newchrled_dev *dev = (struct newchrled_dev*)filp->private_data;
    return 0;
}

static ssize_t newchrled_write(struct file *filp,const char __user *buf,size_t count,loff_t *ppos)
{
    int retvalue;
    unsigned char databuf[1];

    retvalue = copy_from_user(databuf,buf,count);
    if(retvalue < 0) {
        printk("kernel write failed\r\n");
        return -EFAULT;
    }
    /*led is open or close*/
    led_switch(databuf[0]);
    return 0;
}



static const struct file_operations newchrled_fops = {
    .owner = THIS_MODULE,
    .write = newchrled_write,
    .open  = newchrled_open,
    .release = newchrled_release,

};

static int led_probe(struct platform_device *dev)
{
        int i = 0;
        struct resource *ledsource[5];

        int ret = 0;
        unsigned int val = 0;

        printk("led driver probe\r\n");
        /*初始化LED，字符设备驱动*/
        /*1、从设备中获取资源*/
        for (i = 0; i < 5; i++)
        {
                ledsource[i] = platform_get_resource(dev,IORESOURCE_MEM,i);  
                if (ledsource[i] == NULL) {
                        return -EINVAL;
                } 

                //ledsource[0]->end -ledsource[0]->start + 1;    /*长度计算 resource_size()下面有个函数可以替换这个*/
                /*内存映射*/
        }


        /*1.io init ioremap*/
        /*32bit->4*/
        IMX6U_CCM_CCGR1         = ioremap(ledsource[0]->start,resource_size(ledsource[0]));
        SW_MUX_GPIO1_IO03       = ioremap(ledsource[1]->start,resource_size(ledsource[1]));
        SW_PAD_GPIO1_IO03       = ioremap(ledsource[2]->start,resource_size(ledsource[2]));
        GPIO1_DR                = ioremap(ledsource[3]->start,resource_size(ledsource[3]));
        GPIO1_GDIR              = ioremap(ledsource[4]->start,resource_size(ledsource[4]));

        /*2.gpio init*/
        val = readl(IMX6U_CCM_CCGR1);
        val &= ~(3 << 26);/*clear old data bit26 27*/
        val |= 3 << 26; /*set bit26 27 == 1*/
        writel(val,IMX6U_CCM_CCGR1);

        writel(0x5,SW_MUX_GPIO1_IO03);/*setting fuyong*/
        writel(0x10b0,SW_PAD_GPIO1_IO03);/*setting dianqisuxing*/

        val = readl(GPIO1_GDIR);
        val |= 1 << 3; /*output == 1*/
        writel(val,GPIO1_GDIR);

        val = readl(GPIO1_DR);
        // val &= ~(1 << 3); /*led is open while GPIO1_DR is 0 but I want to it is close if begin*/
        val |= (1 << 3);
        writel(val,GPIO1_DR);

        /*2.register device*/
        if(newchrled.major) {/*down is major and minor number*/
                newchrled.devid = MKDEV(newchrled.major,0);
                ret = register_chrdev_region(newchrled.devid,PLATFORM_COUNT,PLATFORM_NAME);
        } else {        /*no give device number*/
                ret = alloc_chrdev_region(&newchrled.devid,0,PLATFORM_COUNT,PLATFORM_NAME);
                newchrled.major = MAJOR(newchrled.devid);
                newchrled.minor = MINOR(newchrled.devid);
        }
        if (ret < 0) {
                printk("newchrled chrdev_region err\r\n");
                return -1;
        }
        printk("newchrled major = %d minor = %d\r\n",newchrled.major,newchrled.minor);

        /*3.register device*/
        newchrled.cdev.owner = THIS_MODULE;
        cdev_init(&newchrled.cdev,&newchrled_fops);
        ret = cdev_add(&newchrled.cdev,newchrled.devid,PLATFORM_COUNT);

        /*4.auto create dev */
        newchrled.class = class_create(THIS_MODULE,PLATFORM_NAME);
        if (IS_ERR(newchrled.class)) {
                return PTR_ERR(newchrled.class);
        }
        newchrled.device = device_create(newchrled.class,NULL,newchrled.devid,NULL,PLATFORM_NAME);

        if (IS_ERR(newchrled.device)) {
                return PTR_ERR(newchrled.device);
        }

        return 0;
}

static int led_remove(struct platform_device *dev)
{

        unsigned int val = 0;
        val = readl(GPIO1_DR);
        val |= (1 << 3); /*led is open while GPIO1_DR is 0*/
        writel(val,GPIO1_DR);

        printk("led driver remove\r\n");
        /*iounmap*/
        iounmap(IMX6U_CCM_CCGR1);
        iounmap(SW_MUX_GPIO1_IO03);
        iounmap(SW_PAD_GPIO1_IO03);
        iounmap(GPIO1_DR);
        iounmap(GPIO1_GDIR);

        cdev_del(&newchrled.cdev);
        /*1.delete device nunber*/
        unregister_chrdev_region(newchrled.devid,PLATFORM_COUNT);
        
        /*3*/
        device_destroy(newchrled.class,newchrled.devid);
        /*4*/
        class_destroy(newchrled.class);
        return 0;
}
/*platform驱动结构体*/
static struct platform_driver led_driver = {
        .driver         = {
                .name  = "imx6ull-led",/*驱动名字，用于和设备匹配 匹配成功就会执行led_probe函数*/
        },
        .probe          = led_probe,
        .remove         = led_remove,

};


/*设备加载*/
static int __init leddriver_init(void)
{
        int ret = 0;
        printk("leddriver_init\r\n");

        /*注册platform驱动*/
        ret = platform_driver_register(&led_driver);
        return ret;
}

/*设备卸载*/
static void __exit leddriver_exit(void)
{
        printk("leddriver_exit\r\n");
        /*卸载platform驱动*/
        platform_driver_unregister(&led_driver);


}


module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("liutao");
```

