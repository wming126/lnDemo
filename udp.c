/**
    @file       udp.c
    @brief      VxWorks下udp测试程序
    @copyright  senbo
    @author     nick.xu
    @version    V1.0
    @date       2017.10.13 V1.0 创建
    @note       程序用来测试udp点播，组播，广播
*/

#include "vxWorks.h"
#include "stdio.h"
#include "unistd.h"
#include "ioLib.h"
#include "fcntl.h"
#include "taskLib.h"
#include "in.h"
#include "iv.h"
#include "sioLib.h"
#include "string.h"
#include "logLib.h"
#include "tickLib.h"
#include "sockLib.h"
#include "inetLib.h"
#include "getOptLib.h" /* vx5.5增加 */

/**
参数结构体, 程序需要用的参数组成一个结构体, 
这样可以解决参数传递过多问题.
*/
typedef struct Udp_s
{
    int mode;
    int type;
    int port;
    int ip;
}Udp_t;

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

static int send_data(Udp_t *pUdp);
static int receive_data(int ip, int port);

static int print_usage()
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

static int parse_usage(int argc, char *argv[], Udp_t* pUdp)
{
    int ret = 0;
    int valid = 0;

    optind = 1;
    optreset = 1;
    while ((ret = getopt(argc, argv, "r:w:p:m:")) != -1)
    {
        switch (ret)
        {
        case 'r':
            pUdp->mode = 0;
            pUdp->port = strtoul(optarg, NULL, 10);
            valid++;
            break;
        case 'w':
            pUdp->mode = 1;
            pUdp->port = strtoul(optarg, NULL, 10);
            valid++;
            break;
        case 'p':
            pUdp->type = 0;
            pUdp->ip = inet_addr(optarg);
            break;
        case 'm':
            pUdp->type = 1;
            pUdp->ip = inet_addr(optarg);
            break;
        }
    }
    
    /* 没有参数或参数不对或缺参数 */
    if (optind == 1 || optopt == '?')
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
    @fn         int udp(char *argv)
    @brief      串口测试函数
    @author     nick.xu
    @param[in]  MultiAddr   char*   组播地址
    @param[in]  index       int     网卡设备索引(0-1:gei0-gei7)
    @param[in]  port        int     端口号(0:8082)
    @retval     0 成功
    @retval     -1 失败
    @note       函数运行后会接收组播地址数据。
*/
int udp(char *string)
{
    int ret = 0;
    Udp_t udp;
    char *tempArgv[64];
    char **argv = tempArgv;
    char newString[64];
    int argc = 0;
    
    memset(&udp, 0x00, sizeof(Udp_t));
    
    strcpy(newString, string);
    ret = getOptServ (newString, "udp", &argc, argv, 64);
    if (ret !=0)
    {
        ret = -1;
        goto Exit;
    }
    
    ret = parse_usage(argc, argv, &udp);
    if (ret != 0)
    {
        ret = -2;
        goto Exit;
    }
    
    printf("%s %s ip=0x%x port=%d\n", s_string[udp.mode], s_string2[udp.type], 
            udp.ip, udp.port);

    if (udp.mode)
    {
        ret = send_data(&udp);
    }
    else
    {
        ret = taskSpawn("tUdp", 100, 0, 1024 * 40, (FUNCPTR)receive_data, udp.ip, udp.port, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    
Exit:
    return ret;
}

static int send_data(Udp_t *pUdp)
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
    if (fd == ERROR)
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
    remote.sin_port = htons(pUdp->port);
    remote.sin_addr.s_addr = pUdp->ip;
    
    ret = sendto(fd, (char *)buffer, 256, 0, (struct sockaddr *)&remote, sizeof(struct sockaddr_in));

    taskDelay(sysClkRateGet() / 2);    /* VxWorks sendto函数有bug 无法阻塞到数据发送完毕 */

    printf("sent = %d\n", ret);

    /* 关闭套接字 */
    if (fd != 0)
    {
        close(fd);
    }

Exit:
    return ret;
}

static int receive_data(int ip, int port)
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
    if (fd == ERROR)
    {
        printf("socket failed!%d\n", errno);
        ret = -1;
        goto Exit;
    }

    /* 必须清零 */
    memset(&local, 0x00, sizeof(struct sockaddr_in));
    memset(&remote, 0x00, sizeof(struct sockaddr_in));

    local.sin_family = AF_INET;
    local.sin_port = htons(port); /* 监听端口号 */
    local.sin_addr.s_addr = ip;   /* 通过IP地址选择网卡 */

    /* 绑定端口 */
    if (bind(fd, (struct sockaddr *)&local, socketLength) == -1)
    {
        printf("bind failed!%d\n", errno);
        ret = -2;
        goto Exit;
    }

    printf("udp receiving in background, ip=0x%x port=%d.\n", ip, port);

    /* 显示接收数据 */
    for (sum = 0; ; sum += length)
    {
        length = recvfrom(fd, (char *)buffer, sizeof(buffer), 0, (struct sockaddr *)&remote, &socketLength);
        if (length == ERROR)
        {
            printf("recvfrom failed!\n");
            break;
        }

        /* 打印接收到数据 */
        printf("--- ");
        for (i = 0; i < length; i++)
        {
            printf("%02X ", buffer[i]);
            if (((i + 1) % 16) == 0) printf("\n    "); /* 16个一换行 */
        }
        printf("--- udp server port=%d\n", port);
    }

Exit:

    if (fd != -1)
    {
        close(fd);
    }

    return ret;
}
