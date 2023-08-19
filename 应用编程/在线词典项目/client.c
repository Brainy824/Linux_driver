#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define N 32

#define R 1     //定义信息类型  user-register
#define L 2     //用户登录      user-login
#define Q 3     //查询          user-query
#define H 4     //历史记录      user-history

/*通信双方的信息结构体*/
typedef struct {
    int type;//类型
    char name[N];
    char data[256];

}MSG;

//注册函数
int do_register(int sockfd, MSG *msg)
{
    //姓名
    printf("register ...\n");
    msg->type = R;
    printf("Input name:");
    scanf("%s", msg->name);
    getchar();//干掉垃圾字符

    //密码
    printf("Input passwd:");
    scanf("%s", msg->data);
    
    //发送给服务器
    if (send(sockfd, msg, sizeof(MSG), 0) < 0) {
        //小于0 发送失败
        printf("fail to send.\n");
        return -1;//退出
    }
    
    //注册成功之后
    if (recv(sockfd, msg, sizeof(MSG), 0) < 0) {
        //小于0 代表读取失败
        printf("fail to recv.\n");
        return -1;//进程退出
    }

    //接收到对方的消息之后，应该打印以下结果
    //ok! or usr alread exit
    printf("%s\n", msg->data);

    return 0;
}
/**
 * @brief 登录函数 有两种返回类型
 * 
 * @param sockfd 
 * @param msg 
 * @return int 
 */
int do_login(int sockfd, MSG *msg)
{
    //姓名
    msg->type = L;
    printf("login ...\n");
    printf("Input name:");
    scanf("%s", msg->name);
    getchar();//干掉垃圾字符

    //密码
    printf("Input passwd:");
    scanf("%s", msg->data);

    //发送给服务器
    if (send(sockfd, msg, sizeof(MSG), 0) < 0) {
        //小于0 发送失败
        printf("fail to send.\n");
        return -1;//退出
    }

    //注册成功之后
    if (recv(sockfd, msg, sizeof(MSG), 0) < 0) {
        //小于0 代表读取失败
        printf("fail to recv.\n");
        return -1;//进程退出
    }

    //等待服务器返回数据
    //比较
    if (strncmp(msg->data, "OK", 3) == 0) {
        //登录成功
        printf("login ok.\n");
        return 1;
    } else {
        printf("%s\n", msg->data);
    }

    return 0;
}

int do_query(int sockfd, MSG *msg)
{
    printf("do_query ...\n");
    msg->type = Q;
    puts("---------------------");
    
    while (1) {
        printf("Input word:");
        scanf("%s", msg->data);
        getchar();//吃掉\n的回车

        //# 代表退出 比较1个字符 客户端，输入#号，返回到上一级菜单
        if (strncmp(msg->data, "#", 1) == 0) {
            break;//跳出while循环
        }

        //发送 将要查询的单词发送给服务器
        if (send(sockfd, msg, sizeof(MSG), 0) < 0) {
            printf("fail to send.\n");
            return -1;
        }

        //接收 等待接收服务器，传递回来的单词的注释信息
        if (recv(sockfd, msg, sizeof(MSG), 0) < 0) {
            printf("fail to recv.\n");
            return -1;
        }

        //将接收的信息打印处理 也就是注释
        printf("%s\n", msg->data);
    }

    printf("go out\n");

    return 0;
}
int do_history(int sockfd, MSG *msg)
{
    printf("do_history ...\n");
    msg->type = H;

    send(sockfd, msg, sizeof(MSG), 0);

    //接收服务器，传递回来的历史记录信息
    while(1) {
        recv(sockfd, msg, sizeof(MSG), 0);

        if (msg->data[0] == '\0'){
            break;
        }
        //输出历史记录 信息
        printf("history:%s\n", msg->data);
    }

    return 0;
}
/**
 * @brief client 程序
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, const char *argv[])
{
    int sockfd;//文件描述符
    int n;
    MSG msg;

    struct sockaddr_in serveraddr;//网络结构体 服务器地址

    //1.判断传参是否满足条件
    if (argc != 3) {
        printf("usage:%s serverip port.\n", argv[0]);
        return -1;
    }

    //2.创建一个socket套接字
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("fail to socket.\n");
        return -1;
    }

    //3.对服务器的地方进行清空
    bzero(&serveraddr, sizeof(serveraddr));

    //4.填充网络结构体
    serveraddr.sin_family = AF_INET;    
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]); //将我们输入的ip地址字符串转换为网络地址二进制
    serveraddr.sin_port = htons(atoi(argv[2]));  //将本机的数据转换为网络数据 (端口转换为整形数据)

    //5.连接服务器
    if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("fail to connect");
        return -1;
    }

    //6.一级菜单
    while (1) {
        printf("****************************************\n");
        printf("*     1.register  2.login   3.quit     *\n");
        printf("****************************************\n");
        printf("please choose:");

        scanf("%d", &n);
        getchar();//去掉垃圾字符

        switch (n) {
            case 1:
                do_register(sockfd, &msg);//注册函数
                break;
            case 2:
                if (do_login(sockfd, &msg) == 1) {//登录函数
                    goto next;
                }
                break;
            case 3:
                close(sockfd);
                exit(0);
                break;
            default:
                printf("Invalid data cmd.\n");
        }

    }


next:
    while (1) {
        printf("****************************************\n");
        printf("* 1.query_word   2.history    3.quit   *\n");
        printf("****************************************\n");
        printf("please choose:");

        scanf("%d", &n);
        getchar();//回收

        switch (n) {
            case 1:
                do_query(sockfd, &msg);
                break;
            case 2:
                do_history(sockfd, &msg);
                break;
            case 3:
                close(sockfd);
                exit(0);
                break;
            default:
                printf("Invalid data cmd.\n");
        }

    }
    return 0;
}