[toc]

## 安装kernel

##### 1、为什么要在Ubuntu下解压kernel，要在vscode中去更改路径，后面驱动需要编译的路径

什么是符号链接，其实就是快捷方式（软链接）

##### 2、构建的根文件系统通过nfs挂载（busybox)

##### 3、将NXP官方的内核源码解压/home/local/linux/IMX6ULL/linux

##### 4、将编译后的文件直接存储载/home/local/linux/nfs/rootfs下，然后添加交叉编译器里面的一些库文件

## 总结

首先将正点原子的uboot和内核源码镜像都编译后烧入到开发板（具体的那些网络配置可以自己去看看），将这两点弄好后，就是开始配置根文件系统了。busybox那个压缩包里面。（期间问题，测试时别写错驱动代码，init和exit)

