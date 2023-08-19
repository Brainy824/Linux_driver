#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sqlite3.h>
#include <signal.h>
#include <time.h>

#define N       32

#define R       1     //定义信息类型  user-register
#define L       2     //用户登录      user-login
#define Q       3     //查询          user-query
#define H       4     //历史记录      user-history

//数据库的宏
#define DATABASE    "my.db"

/*通信双方的信息结构体*/
typedef struct {
    int type;//类型
    char name[N];
    char data[256];

}MSG;

int do_client(int acceptfd, sqlite3 *db);
//注册函数
int do_register(int acceptfd, MSG *msg, sqlite3 *db);
int do_login(int acceptfd, MSG *msg, sqlite3 *db);
int do_query(int acceptfd, MSG *msg, sqlite3 *db);
int do_history(int acceptfd, MSG *msg, sqlite3 *db);
int do_searchword(int acceptfd, MSG *msg, char word[]);
int get_date(char *date);
int history_callback(void *arg, int f_num, char **f_value, char **f_name);


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
    sqlite3 *db;
    int acceptfd; //并发服务器
    pid_t pid;   //父子进程

    struct sockaddr_in serveraddr;//网络结构体 服务器地址

    //1.判断传参是否满足条件
    if (argc != 3) {
        printf("usage:%s serverip port.\n", argv[0]);
        return -1;
    }
    //打开数据库
    if (sqlite3_open(DATABASE, &db) != SQLITE_OK) {
        printf("%s\n", sqlite3_errmsg(db));
    } else {
        printf("open DATABASE success.\n");
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

    /*5.绑定*/
    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("fail to bind.\n");
        return -1;
    }
    
    //绑定成功以后 将套接字设为监听模式
    if (listen(sockfd, 5) < 0) {
        printf("fail to listen.\n");
        return -1;
    }

    //僵尸进程的处理 直接忽略
    signal(SIGCHLD, SIG_IGN);

    //6.创建父子进程
    while (1) {
       if ((acceptfd = accept(sockfd, NULL, NULL)) < 0) {
            perror("fail to accept");
            return -1;
       }

       if ((pid = fork()) < 0) {
            perror("fail to fork");
            return -1;
       } else if (pid == 0) { //儿子进程
            //处理客户段具体的消息 不需要监听
            close(sockfd);
            do_client(acceptfd, db);

       } else { //父亲进程， 用来接收客户端的请求的
            close(acceptfd);

       }

    }


    return 0;
}


int do_client(int acceptfd, sqlite3 *db)
{
    MSG msg;
    while (recv(acceptfd, &msg, sizeof(msg), 0) > 0) {
        printf("type:%d\n", msg.type);
        switch (msg.type) {
            case R:
                do_register(acceptfd, &msg, db);
                break;
            case L:
                do_login(acceptfd, &msg, db);
                break;
            case Q:
                do_query(acceptfd, &msg, db);
                break;
            case H:
                do_history(acceptfd, &msg, db);
                break;
            default:
                printf("Invalid mag.\n");
        }
    }

    printf("client exit.\n");
    close(acceptfd);
    exit(0);

    return 0;
}

//注册函数
int do_register(int acceptfd, MSG *msg, sqlite3 *db)
{
    printf("register ...\n");
    char *errmsg;
    char sql[128];

    sprintf(sql, "insert into usr values('%s', %s);", msg->name, msg->data);
    printf("%s\n", sql);


    if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
        //认为插入失败
        printf("%s\n", errmsg);
        strcpy(msg->data, "usr name already exist.");
    } else {
        //插入成功
        printf("client register ok\n");
        strcpy(msg->data, "OK!");
    }

    //给客户端回一个信息
    if (send(acceptfd, msg, sizeof(MSG), 0) < 0) {
        perror("fail to send");
        return 1;
    }
    

    return 0;

}
/**
 * @brief 登录函数 有两种返回类型
 * 
 */
int do_login(int acceptfd, MSG *msg, sqlite3 *db)
{

    char sql[128] = {0};//数据库的查询
    char *errmsg;
    int nrow;//行数
    int ncloumn;//列数
    char **resultp;
    printf("login ...\n");

    sprintf(sql, "select * from usr where name = '%s' and pass = '%s';", msg->name, msg->data);
    printf("%s\n", sql);

    //用于查询
    if (sqlite3_get_table(db, sql, &resultp, &nrow, &ncloumn, &errmsg) != SQLITE_OK) {
        //查询失败
        printf("%s\n", errmsg);
        return -1;
    } else {
        //这里sqlite3_get_table成功执行了
        printf("get_table ok!\n");
    }

    //查询成功  服务器数据库中拥有此用户
    if (nrow == 1) {
        strcpy(msg->data, "OK");
        send(acceptfd, msg, sizeof(MSG), 0);
        return 1;
    } 

    if (nrow == 0) { //密码或者用户名错误
        strcpy(msg->data, "usr/passwd wrong.");
        send(acceptfd, msg, sizeof(MSG), 0);        
    }

    return 0;
}

//查询的结果
int do_searchword(int acceptfd, MSG *msg, char word[])
{
    FILE *fp;
    int len = 0;

    char temp[512] = {0}; //读这一行
    int result = 0;
    char *p; //找到单词了，指针指向

    //打开文件，读取文件，进行比对
    
    if ((fp = fopen("./dict.txt", "r")) == NULL) { //打开失败
        perror("fail to fopen.\n");
        strcpy(msg->data, "Failed to open dict.txt");
        send(acceptfd, msg, sizeof(MSG), 0);
        return -1;
    }

    //打开成功 打印出客户端要查询的单词 并计算单词的长度
    len = strlen(word);
    printf("%s, len = %d\n", word, len);

    //读文件， 来查询单词 一次读一行
    while(fgets(temp, 512, fp) != NULL) { //不等于空，继续读

        result = strncmp(temp, word, len);//比较 返回值是字符大小 
        //printf("result: %d, temp: %s\n", result, temp);

        if (result < 0) {//表示含义 往下一行读取
            continue;
        }

        if (result > 0 || ((result == 0) && (temp[len] != ' '))) { //单词表中单词最后都是空格
            break;
        }

        //表示找到了，查询的单词
        p = temp + len; //abandon    v. adksd sadkfj 表示指向了了这个单词的末尾
        //printf("found word:%s\n", p);
        //去掉空格
        while(*p == ' ') { //让这个指针向后移动 向后移动的结果就是第一次出现不等于空格的时候跳出来
            p++;
        }

        //跳出来之后 找到了注释，跳跃过所有的空格
        strcpy(msg->data, p);//将p中的数据拷贝到msg->data中
        printf("found word:%s\n", msg->data);

        //拷贝完毕之后 应该关闭文件
        fclose(fp);
        return 1;
    }
    fclose(fp);
    return 0;
}

//获取时间函数
int get_date(char *date)
{
    time_t t;
    struct tm *tp;

    time(&t);//获取1970-01-01到现在的秒数

    //进行时间的格式转换
    tp = localtime(&t);//这里接收的返回值 时间的返回值

    sprintf(date, "%d-%d-%d %d:%d:%d", tp->tm_year + 1900 ,  tp->tm_mon + 1, tp->tm_mday, 
                                            tp->tm_hour, tp->tm_min, tp->tm_sec);
    printf("get date:%s\n", date);
    return 0;
}   

//查询
int do_query(int acceptfd, MSG *msg, sqlite3 *db)
{
    int ret = 0;
    printf("do_query ...\n");
    char word[64];//传递进来的单词给到64个字节
    int found = 0;//查询到的情况，分两种情况
    char sql[128] = {0};//拼接sql 插入到rcord表中去
    
    char date[128] = {0};//系统时间

    char *errmsg;

    //拿出msg结构体中，要查询的单词
    strcpy(word, msg->data);

    //定义以查询函数
    found = do_searchword(acceptfd, msg, word);
    printf("found:%d\n", found);
    printf("查询一个单词完毕\n");

    //表示找到了单词，此时应该将 用户名，时间，单词，插入到历史记录表中去
    if (found == 1) { 
        //需要获取系统时间
        ret = get_date(date);
        printf("get date:%d\n", ret);
        printf("%s\n", date);//这里也打印以下系统时间值

        //字符串拼接
        sprintf(sql, "insert into record values('%s', '%s', '%s')", msg->name, date, word);
        printf("%s\n", sql);

        if (sqlite3_exec(db, sql, NULL, NULL, &errmsg) != SQLITE_OK) {
            //不等于表示插入失败
            printf("%s\n", errmsg);
            return -1;
        } else {
            printf("Insert record done.\n");
        }


    } else { //表示没有找到
        strcpy(msg->data, "Not found!");
    }

    //将查询的结果，发送给客户端 同一进行传递
    send(acceptfd, msg, sizeof(MSG), 0);
    printf("send: %s\n", msg->data);

    return 0;
}
//回调函数
/**
 * @brief sqlite3_exec回调函数 得到查询结果 并且需要将历史记录发送给客户端
 * 
 * @param arg 
 * @param f_num 查询的列数
 * @param f_value 查询的结果
 * @param f_name 查询的名字
 * @return int 
 */
int history_callback(void *arg, int f_num, char **f_value, char **f_name)
{
    //record ,   f_value[] =  name[0], date[1], word[2]
    int acceptfd = 0;
    MSG msg;
    
    acceptfd = *((int *)arg);//强制转换

    sprintf(msg.data, "%s, %s", f_value[1], f_value[2]);

    send(acceptfd, &msg, sizeof(MSG), 0);

    return 0;

}

int do_history(int acceptfd, MSG *msg, sqlite3 *db)
{
    printf("do_history ...\n");
    char sql[128] = {0};
    char *errmsg;

    //拼接sql语句
    sprintf(sql, "select * from record where name = '%s'", msg->name);

    //查询数据库 需要给callback传参
    if (sqlite3_exec(db, sql, history_callback, (void *)&acceptfd, &errmsg) != SQLITE_OK) {
        //表示查询失败
        printf("%s\n", errmsg);
    } else { //成功
        printf("query record done\n");
    }


    //所有的记录查询 发送完毕之后，给客户端发送一个接收信息
    //这样就不会阻塞在查询历史记录模块这里了
    msg->data[0] = '\0';
    send(acceptfd, msg, sizeof(MSG), 0);

    return 0;
}
