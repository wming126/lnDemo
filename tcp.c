/**
    @file       tcp.c
    @brief      Linux下tcp测试程序
    @copyright  senbo
    @author     wming
    @version    V1.0
    @date       2017.10.31 V1.0 创建
    @note       程序用来测试tcp server和client
*/

#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "string.h"
#include "errno.h"
#include "termios.h"

#include "sys/mman.h"
#include "sys/ioctl.h"

#include "sys/socket.h"
#include "netinet/in.h"
#include "arpa/inet.h"
#include "signal.h"

#define DEBUG     0

static char * pServer = NULL;
static char * pClient = NULL;

/**
参数结构体, 程序需要用的参数组成一个结构体,
这样可以解决参数传递过多问题.
*/
typedef struct Para_s
{
    int mode;
    int port;
    int ip;
    unsigned long count;
} Para_t;

static char *s_string[] =
{
    "Client",
    "Server",
};

static int tcp_server(Para_t *pPara);
static int tcp_client(Para_t *pPara);

/**
    @fn         static int print_usage(void)
    @brief      打印程序用法
    @author     nick.xu
    @retval     0 成功
    @retval     -1 失败
    @note       打印程序用法供使用者参考
*/
static int print_usage(void)
{
    printf("Usage: tcp -[sc] <ip> <port> \n"
           "\t-s: tcp server\n"
           "\t-c: tcp client\n"
           "\t- : ip address 192.168.1.101\n"
           "\t-p: server or client port\n"
           "Example: tcp -s 192.168.1.200 -p 8080\n"
           "Example: tcp -c 192.168.1.200 -p 8080\n"
          );

    return 0;
}

/**
    @fn         static int parse_usage(int argc, char *argv[], Para_t* pPara)
    @brief      解析命令行参数
    @author     nick.xu
    @param[in]  argc        int         参数个数
    @param[in]  argv        char**      参数指针数组
    @param[in]  pPara       Para_t      内部参数结构体
    @retval     0 成功
    @retval     -1 失败
    @note       函数使用getopt函数对参数进行解析, 把正确的值写入结构体.
*/
static int parse_usage(int argc, char *argv[], Para_t *pPara)
{
    int ret = 0;
    int valid = 0;

    while ((ret = getopt(argc, argv, "s:c:p:n:")) != -1)
    {
        switch (ret)
        {
        case 's':
            pPara->mode = 1;
            pPara->ip = inet_addr(optarg);
            valid++;
            break;
        case 'c':
            pPara->mode = 0;
            pPara->ip = inet_addr(optarg);
            valid++;
            break;
        case 'n':
            pPara->count = strtoul(optarg, NULL, 10);
#if DEBUG
            printf("pPara->count:%u \n",pPara->count);
#endif
            break;
        case 'p':
            pPara->port = strtoul(optarg, NULL, 10);
#if DEBUG
            printf("pPara->port:%d \n",pPara->port);
#endif
            break;
        }
    }

    /* 没有参数 */
    if (optind == 1)
    {
        print_usage();
        return -1;
    }

    /* 参数不符合逻辑 */
    if ((valid != 1))
    {
        print_usage();
        return -1;
    }

    return 0;
}

/**
    @fn         int signal_sigint(int signo)
    @brief      信号捕捉函数
    @author     wming
    @param[in]  signo       int     固定格式
    @retval     void
*/
void signal_sigint(int signo)
{
    /* todo 关闭所有socket连接  */

    printf("The program will be exit!\n");

    exit(0);
}

/**
    @fn         int main(int argc, char *argv[])
    @brief      tcp测试函数
    @author     wming
    @param[in]  argc        int         参数个数
    @param[in]  argv        char**      参数指针数组
    @retval     0 成功
    @retval     -1 失败
    @note       函数根据模式分别调用发送和接收函数。
*/
int main(int argc, char *argv[])
{
    int ret = 0;
    Para_t para;

    /* 注册ctrl+C信号*/
    signal(SIGINT, signal_sigint);

    /* 默认参数 */
    memset(&para, 0x00, sizeof(Para_t));
    para.port = 8080;

    /* 解析参数 */
    ret = parse_usage(argc, argv, &para);
    if (ret != 0)
    {
        ret = -1;
        goto Exit;
    }

    printf("%s ip=0x%x port=%d\n", s_string[para.mode],para.ip, para.port);

    if (para.mode)
    {
        ret = tcp_server(&para);
    }
    else
    {
        ret = tcp_client(&para);
    }

Exit:
    return ret;
}

/**
    @fn         static int tcp_server(Para_t *pPara)
    @brief      创建TCP server 并等待接受数据
    @author     wming
    @param[in]  pPara       Para_t      内部参数结构体
    @retval     0 成功
    @retval     -1 失败
    @note       函数先创建soeckt, 然后等待接受数据.
*/
static int tcp_server(Para_t *pPara)
{
    int fd_server = 0;
    int fd_client = 0;
    unsigned char buffer[256];
    int i = 0;
    int ret = 0;
    struct sockaddr_in server;
    struct sockaddr_in client;
    socklen_t socketLength = sizeof(struct sockaddr_in);
    int length = 0;
    unsigned long sum = 0;
    int size = sizeof(buffer);

    /* 必须清零 */
    memset(&server, 0x00, sizeof(struct sockaddr_in));
    memset(&client, 0x00, sizeof(struct sockaddr_in));

    /* 创建套接字 */
    fd_server = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_server == -1)
    {
        printf("socket failed!%d\n", errno);
        ret = -1;
        goto Exit;
    }

#if DEBUG
    printf("The socket creat succeed!\n");
#endif

    server.sin_family = AF_INET;
    server.sin_port = htons(pPara->port);
    server.sin_addr.s_addr = pPara->ip;

    if(bind(fd_server, (struct sockaddr*)&server, sizeof(struct sockaddr)) == -1 )
    {
        perror("bind err");
        goto Exit;
    } 

#if DEBUG
    printf("Bind succeed!\n");
#endif

    // 只支持一个连接
    if(listen(fd_server, 1) == -1)
    {
        perror("Listen err");
        goto Exit;
    }

#if DEBUG
    printf("The server is listenning...\n");
    printf("Before accept:socktfd_client is %d\n",fd_client);
#endif

    for(;;)
    {
        printf("press ctrl+c to quit.\n");
        if( (fd_client = accept(fd_server, (struct sockaddr*)&client,&socketLength)) == -1 )
        {
            perror("accept err");
            goto Exit;
        }
        
        printf("--- ");
        for(sum = 0 ; ; sum+=length)
        {
            memset(buffer, 0x00, sizeof(buffer));


            length = recv(fd_client,buffer,size, 0);

            // 当client关闭连接时recv返回0，当发生错误时返回-1，无论哪种情况都跳转到accept
            if(length <= 0)
            {
                break;
            }

            for (i = 0; i < length; i++)
            {
                printf("%02X ", buffer[i]);
                if (((i + 1) % 16) == 0) printf("\n    "); /* 16个一换行 */
            }

            /* 回应接收到的数据 */
            sendto(fd_client, buffer, length, 0, (struct sockaddr *)&server, socketLength);
        }
        printf("---sum%lu tcp port=%d\n", sum, pPara->port);
    }

Exit:
    /* 关闭套接字 */
    if (fd_server != 0)
    {
        close(fd_server);
    }

    return ret;
}

/**
    @fn         static int tcp_client(Para_t *pPara)
    @brief      创建TCP server 并等待接受数据
    @author     wming
    @param[in]  pPara       Para_t      内部参数结构体
    @retval     0 成功
    @retval     -1 失败
    @note       函数先创建soeckt然后链接到server发送数据
*/
static int tcp_client(Para_t *pPara)
{
    int ret = 0;
    int fd_client = 0;
    struct sockaddr_in server;
    struct sockaddr_in client;
    unsigned char* buffer;
    long length = 0;
    unsigned long sum = 0;
    unsigned long i = 0;
    socklen_t socketLength = sizeof(struct sockaddr_in);
    int size = pPara->count % 10000000;
    struct timeval tv;

    /* 根据命令行参数动态申请buffer */
    if((buffer = malloc(size)) == NULL)
    {
        perror("malloc err");
        exit(1);
    }

    memset(buffer, 0x00, size);

    for(i = 0; i < size; i++)
    {
        buffer[i] = i%256;
    }

    /* 创建套接字 */
    fd_client = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_client == -1)
    {
        printf("socket failed!%d\n", errno);
        ret = -1;
        goto Exit;
    }

#if DEBUG
    printf("The sockte creat succeed!\n");
#endif

    /* 必须清零 */
    memset(&client, 0x00, sizeof(struct sockaddr_in));
    memset(&server, 0x00, sizeof(struct sockaddr_in));

    client.sin_family = AF_INET;
    client.sin_port = htons(pPara->port); /* 监听端口号 */
    client.sin_addr.s_addr = pPara->ip;   /* 通过IP地址选择网卡 */


    /* 设置接受超时时间 */
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    if(setsockopt(fd_client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("setsockopt err");
        goto Exit;
    }

    if(connect(fd_client, (struct sockaddr *)&client,sizeof(struct sockaddr)))
    {
        perror("Connect err");
        goto Exit;        
    }

    length = send(fd_client, buffer, size, 0);
    if(length > 0)
    {
        printf("The data is send to the server!\n");
    }

    printf("length = %ld\n",length);

    printf("Wait for a response from server.\n");
    
    printf("--------------------- ");
    for(sum = 0; ; sum += length)
    {
        memset(buffer, 0, size);

        length = recvfrom(fd_client, buffer, size, 0, (struct sockaddr *)&client, &socketLength);

        if(length <= 0)
        {
            printf("---sum = %ld tcp client\n", sum);
            break;
        }

        for (i = 0; i < length; i++)
        {
            printf("%02X ", buffer[i]);
            if (((i + 1) % 16) == 0) printf("\n    ");
        }
    }

Exit:
    if (fd_client != -1)
    {
        close(fd_client);
    }

    return ret;
}

