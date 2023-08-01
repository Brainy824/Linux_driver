[toc]

## 总结

### filezilla一定要记得连接上开发板后，它才能连接上虚拟机，不然连接不上的

### 1、在设备树下添加蜂鸣器的子节点（类似与上个gpio的添加方式）

1、添加pinctrl节点（一定要用6ULL的）因为地址不一样

​	beep所使用的IO为SNVS_TAMPER1,作为GPIO5_IO01.

​	注意！6UL和6ULL的SNVS组IO地址是不同的。

2、添加完成后可以通过以下命令在开发板查看

```C
cd /proc/device-tree/
ls
```

3、以下是添加的设备树程序段，记得一定要在里面屏蔽其他用到这个IO的其他pinctrl等

```C
/ {
	/*beep节点*/
	beep {
		compatible = "ltalientek,beep";
		pinctrl-names = "default";
		pinctrl-0 = <&pinctrl_beep_new>;
		beep-gpios = <&gpio5 1 GPIO_ACTIVE_HIGH>;
		status = "okey";

	};
};

&iomuxc {
	pinctrl-names = "default";
	pinctrl-0 = <&pinctrl_hog_1>;
	imx6ul-evk {
        
		pinctrl_beep_new: beepgrp {
			fsl,pins = <
				MX6ULL_PAD_SNVS_TAMPER1__GPIO5_IO01 	0x10b0
			>;
		};
        
	};
};

```

### 2、完整驱动程序，驱动里面的程序框架(还要记得编写write函数)（应用程序就是原来的）

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
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_irq.h>

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define BEEP_CNT     1
#define BEEP_NAME    "beep"

#define BEEPOFF          0
#define BEEPON           1

/*beep设备结构体*/
struct beep_dev {
        dev_t devid;            /*设备号*/
        int major;
        int minor;

        struct cdev cdev;

        struct class *class;
        struct device *device;

        struct device_node *nd;
        int beep_gpio;
};

struct beep_dev beep; /*LED*/


static int beep_open(struct inode *inode,struct file *filp)
{
        filp->private_data = &beep;
        return 0;
}

static int beep_release(struct inode *inode,struct file *filp)
{
        return 0;
}

static ssize_t beep_write(struct file *filp,const char __user *buf,size_t count,loff_t *ppos)
{
        struct beep_dev *dev = filp->private_data;
        int ret;
        unsigned char databuf[1];

        ret = copy_from_user(databuf,buf,count);
        if (ret < 0) {
                return -EFAULT;
        }

        if (databuf[0] == BEEPON) {
                gpio_set_value(dev->beep_gpio,0);
        } else if (databuf[0] == BEEPOFF) {
                gpio_set_value(dev->beep_gpio,1);
        }

        return 0;
}


/*字符设备操作集*/
static const struct file_operations led_fops = {
        .owner          = THIS_MODULE,
        .open           = beep_open,
        .write          = beep_write,
        .release        = beep_release,
};



/*驱动入口函数*/
static int __init beep_init(void)
{
        /*1、注册字符设备驱动*/
        int ret = 0;
        beep.major = 0;      /*不给定设备号，由系统自动分配*/
        printk("hello liutao led_init\r\n");
        if (beep.major) {            /*给定主设备号*/
                beep.devid = MKDEV(beep.major,0); /*主设备号和次设备号*/
                ret = register_chrdev_region(beep.devid,BEEP_CNT,BEEP_NAME);
        } else {        /*没给定设备号*/
                ret = alloc_chrdev_region(&beep.devid,0,BEEP_CNT,BEEP_NAME);/*第一个参数是申请到的*/
                /*保存申请到的主次设备号*/
                beep.major = MAJOR(beep.devid);
                beep.minor = MINOR(beep.devid);
        }
        if (ret < 0) {
                goto fail_devid;
        }
        printk("beep major = %d, minor = %d\r\n",beep.major,beep.minor);

        /*2、初始化cdev*/
        beep.cdev.owner = THIS_MODULE;
        cdev_init(&beep.cdev,&led_fops);     /*第二个参数是字符设备操作集*/
        
        /*3、添加cdev 其返回值在实际中要做处理*/
        ret = cdev_add(&beep.cdev,beep.devid,BEEP_CNT);
        if (ret) {
                goto fail_cdevadd;
        }

        /*4、创建类*/
        beep.class = class_create(THIS_MODULE,BEEP_NAME);
        if (IS_ERR(beep.class)) {    /*由于是指针类型需要判断是否创建成功*/
                ret = PTR_ERR(beep.class);
                goto fail_class;
        }

        /*5、创建设备 感觉也就是相当于在开发板 ls /dev/ 下出现这个设备*/
        beep.device = device_create(beep.class,NULL,beep.devid,NULL,BEEP_NAME);
        if (IS_ERR(beep.device)) {    /*由于是指针类型需要判断是否创建成功*/
                ret = PTR_ERR(beep.device);
                goto fail_device;
        }


        /*初始化BEEP*/
        /*1、获取节点信息*/
        beep.nd = of_find_node_by_path("/beep");
        if (beep.nd == NULL) {
                ret = -EINVAL;
                goto fail_nd;
        }

        /*2、获取GPIO编号*/
        beep.beep_gpio = of_get_named_gpio(beep.nd,"beep-gpios",0);
        if (beep.beep_gpio < 0) {
                ret = -EINVAL;
                goto fail_nd;
        }

        /*3、申请GPIO 第二个参数只是自己给gpio命的一个名字*/
        ret = gpio_request(beep.beep_gpio,"beep-gpio");
        if (ret) {
                printk("cat't request beep gpio\r\n");
                goto fail_nd;
        }

        /*4、申请成功，设置为输出*/
        ret = gpio_direction_output(beep.beep_gpio,0);
        if (ret < 0) {
                goto fail_set;
        }

        /*5、设置为高电平，不让它响，由应用程序控制更好*/
        gpio_set_value(beep.beep_gpio,1);

        return 0;
fail_set:
        gpio_free(beep.beep_gpio);
fail_nd:
        device_destroy(beep.class,beep.devid);
fail_device:
        class_destroy(beep.class);
fail_class:
        cdev_del(&beep.cdev);
fail_cdevadd:
        unregister_chrdev_region(beep.devid,BEEP_CNT);  /*释放设备号*/
fail_devid:
        return ret;
}

/*驱动出口函数*/
static void __exit beep_exit(void)
{
        printk("liutao led_exit\r\n");

        /*在退出之前关闭beep*/
        gpio_set_value(beep.beep_gpio,1);

        /*注销字符设备驱动*/
        cdev_del(&beep.cdev);
        unregister_chrdev_region(beep.devid,BEEP_CNT);

        /*先创建的类，后创建设备。所以要先销毁设备，再销毁类*/
        device_destroy(beep.class,beep.devid);
        class_destroy(beep.class);

        /*释放gpio*/
        gpio_free(beep.beep_gpio);
}


module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("liutao");
```

### 3、注意这里不用私有数据也是可以的

```C
static ssize_t beep_write(struct file *filp,const char __user *buf,size_t count,loff_t *ppos)
{
        struct beep_dev *dev = filp->private_data;
        int ret;
        unsigned char databuf[1];

        ret = copy_from_user(databuf,buf,count);
        if (ret < 0) {
                return -EFAULT;
        }

        if (databuf[0] == BEEPON) {
                gpio_set_value(dev->beep_gpio,0);
        } else if (databuf[0] == BEEPOFF) {
                gpio_set_value(dev->beep_gpio,1);
        }

        return 0;
}
```

2、不用的情况

```C
static ssize_t beep_write(struct file *filp,const char __user *buf,size_t count,loff_t *ppos)
{
        struct beep_dev *dev = filp->private_data;
        int ret;
        unsigned char databuf[1];

        ret = copy_from_user(databuf,buf,count);
        if (ret < 0) {
                return -EFAULT;
        }

        if (databuf[0] == BEEPON) {
                gpio_set_value(beep.beep_gpio,0);
        } else if (databuf[0] == BEEPOFF) {
                gpio_set_value(beep.beep_gpio,1);
        }

        return 0;
}

```



