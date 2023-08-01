[toc]

## 总结

#### 1、pinctrl子系统

借助pinctrl来设置一个PIN的复用和电气属性。

#### 2、如何添加一个PIN的信息

```C
pinctrl_hog_1:hoggrp-1{

	fsl,pins = <

			MX6UL_PAD_UART1_RTS_B_GPIO1_IO19	0x17059 /*SD1 CD*/

			>;
};

MX6UL_PAD_UART1_RTS_B_GPIO1_IO19      0x0090 0x031c 0x0000 0x5 0x0
<mux_reg  conf_reg  input_reg   mux_mode   input_val>
 0x0090   0x031c    0x0000      0x5         0x0
 IOMUXC父节点首地址 0x020e0000,因此 UART1_RTS_B 这个 PIN 的mux 寄存器地址 就是：0x020e0000+0x0090=0x020e 0090.

conf_reg:0x020e0000+0x031c=0x020e 031c,这个寄存器就是UART1_RTS_B 的电器属性配置寄存器。
    
input_reg,编译为0，表示UART1_RTS_B这个PIN没有input功能。
    
mux_mode:5表示复用为GPIO1_IO19,将其写入0x020e 0090
    
input_val:就是写入input_reg寄存器的值
    
0x17059: 为PIN的电器属性配置寄存器值
```

#### 3、pinctrl驱动



