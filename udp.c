/**
    @file       udp.c
    @brief      Linux下udp测试程序
    @copyright  senbo
    @author     nick.xu
    @version    V1.0
    @date       2017.10.30 V1.0 创建
    @note       程序用来测试udp点播，组播，广播
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

/**
参数结构体, 程序需要用的参数组成一个结构体,
这样可以解决参数传递过多问题.
*/
typedef struct Para_s
{
    int mode;
    int type;
    int port;
    int ip;
} Para_t;

static char *s_string[] =
{
    "Read",
    "Write",
};

static char *s_string2[] =
{
    "point",
    "multi",
    "broad",
};

static int send_data(Para_t *pPara);
static int receive_data(Para_t *pPara);

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
    printf("Usage: udp -[rw] <port> -[pm] <ip> \n"
           "\t-r: recive data\n"
           "\t-w: send data\n"
           "\t-p: send p2p data\n"
           "\t-m: send multi data\n"
           "\tip: ip address 192.168.1.1\n"
           "\tport: listen or remote port\n"
           "Example: udp -w 8080 -p 192.168.1.101\n"
           "Example: udp -w 8080 -m 224.0.0.1\n"
           "Example: udp -w 8080 -p 192.168.1.255\n"
           "Example: udp -r 8080 -p 0\n"
           "Example: udp -r 8080 -p 192.168.1.145\n"
           "Example: udp -r 8080 -m 224.0.0.1\n"
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

    while ((ret = getopt(argc, argv, "r:w:p:m:")) != -1)
    {
        switch (ret)
        {
        case 'r':
            pPara->mode = 0;
            pPara->port = strtoul(optarg, NULL, 10);
            valid++;
            break;
        case 'w':
            pPara->mode = 1;
            pPara->port = strtoul(optarg, NULL, 10);
            valid++;
            break;
        case 'p':
            pPara->type = 0;
            pPara->ip = inet_addr(optarg);
            break;
        case 'm':
            pPara->type = 1;
            pPara->ip = inet_addr(optarg);
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
    @fn         int main(int argc, char *argv[])
    @brief      udp测试函数
    @author     nick.xu
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

    printf("%s %s ip=0x%x port=%d\n", s_string[para.mode], s_string2[para.type],
           para.ip, para.port);

    if (para.mode)
    {
        ret = send_data(&para);
    }
    else
    {
        ret = receive_data(&para);
    }

Exit:
    return ret;
}

/**
    @fn         static int send_data(Para_t *pPara)
    @brief      发送udp数据
    @author     nick.xu
    @param[in]  pPara       Para_t      内部参数结构体
    @retval     0 成功
    @retval     -1 失败
    @note       函数先设置串口配置, 然后发送数据.
*/
static int send_data(Para_t *pPara)
{
    int fd = 0;
    unsigned char buffer[256];
    int i = 0;
    int ret = 0;
    int opt = 0;
    char opt2 = 0;
    struct sockaddr_in remote;
    int socketLength = sizeof(struct sockaddr_in);

    /* 必须清零 */
    memset(&remote, 0x00, sizeof(struct sockaddr_in));

    for (i = 0; i < 256; i++)
    {
        buffer[i] = i;
    }

    /* 创建套接字 */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        printf("socket failed!%d\n", errno);
        ret = -1;
        goto Exit;
    }

    /* 设置广播功能 */
    opt = 1;
    ret = setsockopt (fd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));
    if (ret != 0)
    {
        printf("setsockopt failed(SO_BROADCAST)!%d\n", errno);
        ret = -2;
        goto Exit;
    }

    /* 设置ttl值 */
    opt = 255;
    ret = setsockopt (fd, IPPROTO_IP, IP_TTL, (char *)&opt, sizeof(opt));
    if (ret != 0)
    {
        printf("setsockopt failed(IP_TTL)!%d\n", errno);
        goto Exit;
    }

    opt2 = 255;
    ret = setsockopt (fd, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&opt2, sizeof(opt2));
    if (ret != 0)
    {
        printf("setsockopt failed(IP_MULTICAST_TTL)!%d\n", errno);
        goto Exit;
    }

    remote.sin_family = AF_INET;
    remote.sin_port = htons(pPara->port);
    remote.sin_addr.s_addr = pPara->ip;

    ret = sendto(fd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));
    if (ret != sizeof(buffer))
    {
        printf("sent = %d\n", ret);
        ret = -21;
        goto Exit;
    }

Exit:
    /* 关闭套接字 */
    if (fd != 0)
    {
        close(fd);
    }

    return ret;
}

static int receive_data(Para_t *pPara)
{
    int ret = 0;
    int fd = 0;
    struct sockaddr_in local;
    struct sockaddr_in remote;
    int socketLength = sizeof(struct sockaddr_in);
    unsigned char buffer[256];
    int length = 0;
    int sum = 0;
    int i = 0;

    /* 创建套接字 */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1)
    {
        printf("socket failed!%d\n", errno);
        ret = -1;
        goto Exit;
    }

    /* 必须清零 */
    memset(&local, 0x00, sizeof(struct sockaddr_in));
    memset(&remote, 0x00, sizeof(struct sockaddr_in));

    local.sin_family = AF_INET;
    local.sin_port = htons(pPara->port); /* 监听端口号 */
    local.sin_addr.s_addr = pPara->ip;   /* 通过IP地址选择网卡 */

    /* 绑定端口 */
    if (bind(fd, (struct sockaddr *)&local, socketLength) == -1)
    {
        printf("bind failed!%d\n", errno);
        ret = -2;
        goto Exit;
    }

    /* 打印接收数据 */
    printf("press ctrl+c to quit.\n");
    for (sum = 0; ; sum += length)
    {
        length = recvfrom(fd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&remote, &socketLength);
        if (length == -1)
        {
            printf("recvfrom failed!%d\n", errno);
            break;
        }

        /* 打印接收到数据 */
        printf("--- ");
        for (i = 0; i < length; i++)
        {
            printf("%02X ", buffer[i]);
            if (((i + 1) % 16) == 0) printf("\n    "); /* 16个一换行 */
        }
        printf("--- udp port=%d\n", pPara->port);
    }

    ret = 0;

Exit:
    if (fd != -1)
    {
        close(fd);
    }

    return ret;
}
