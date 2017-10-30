/**
    @file       ttys.c
    @brief      Linux�´��ڲ��Գ���
    @copyright  senbo
    @author     nick.xu
    @version    V1.0
    @date       2017.10.30 V1.0 ����
    @note       �����������Դ��ڷ��������
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

/**
�����ṹ��, ������Ҫ�õĲ������һ���ṹ��, 
�������Խ���������ݹ�������.
*/
typedef struct Para_s
{
    int mode;
    int baud;
    int baud2;
    int number;
    int check;
    char name[64];
    char path[128];
}Para_t;

static char *s_string[] = 
{
    "Read",
    "Write",
};

static char *s_string2[] = 
{
    "none",
    "odd",
    "even",
};

static int send_data(Para_t *pPara);
static int receive_data(Para_t *pPara);

/**
    @fn         static int getch2(void)
    @brief      ��������ʽ�������̨����
    @author     nick.xu
    @retval     ��ȡ�ļ�ֵ
    @note       ��������dos�е�getch����, ��ȡ����ʱ��������, Ӧ���ڰ����������ܵĳ���.
*/
static int getch2(void)
{
    int c = 0;
    struct termios old_opts, new_opts;
    int ret = 0;
    int org_set;

    tcgetattr(STDIN_FILENO, &old_opts);
    new_opts = old_opts;
    new_opts.c_lflag &= ~(ICANON | ECHO | ISIG | ECHOPRT);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
    c = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &old_opts);

    return c;
}

/**
    @fn         static int print_usage(void)
    @brief      ��ӡ�����÷�
    @author     nick.xu
    @retval     0 �ɹ�
    @retval     -1 ʧ��
    @note       ��ӡ�����÷���ʹ���߲ο�
*/
static int print_usage(void)
{
    printf("Usage: ttys -[rw] <device> -[b] <baud> -[n] <number> -c <check>\n"
           "\t-r: recive data\n"
           "\t-w: send data\n"
           "\t-b: baud rate\n"
           "\t-n: send number\n"
           "\t-c: check type 0:none 1:odd 2:even\n"
           "\tdevice: ttyS device path\n"
           "Example: ttys -w ttyS0 -b 115200 -n 256\n"
           "Example: ttys -r ttyS0 -b 115200\n"
           );

    return 0;
}

/**
    @fn         static int parse_usage(int argc, char *argv[], Para_t* pPara)
    @brief      ���������в���
    @author     nick.xu
    @param[in]  argc        int         ��������
    @param[in]  argv        char**      ����ָ������
    @param[in]  pPara       Para_t      �ڲ������ṹ��
    @retval     0 �ɹ�
    @retval     -1 ʧ��
    @note       ����ʹ��getopt�����Բ������н���, ����ȷ��ֵд��ṹ��.
*/
static int parse_usage(int argc, char *argv[], Para_t* pPara)
{
    int ret = 0;
    int valid = 0;

    while ((ret = getopt(argc, argv, "r:w:b:n:c:")) != -1)
    {
        switch (ret)
        {
        case 'r':
            pPara->mode = 0;
            strcpy(pPara->name, optarg);
            sprintf(pPara->path, "/dev/%s", pPara->name);
            valid++;
            break;
        case 'w':
            pPara->mode = 1;
            strcpy(pPara->name, optarg);
            sprintf(pPara->path, "/dev/%s", pPara->name);
            valid++;
            break;
        case 'b':
            pPara->baud = strtoul(optarg, NULL, 10);
            break;
        case 'n':
            pPara->number = strtoul(optarg, NULL, 10);
            if (pPara->number > 1024) pPara->number = 1024;
            break;
        case 'c':
            pPara->check = strtoul(optarg, NULL, 10);
            pPara->check &= 0x03;
            if (pPara->check > 2) pPara->check = 0;
            break;
        default:
            print_usage();
            return -1;
        }
    }
    
    /* û�в��� */
    if (optind == 1)
    {
        print_usage();
        return -1;
    }

    /* �����������߼� */
    if ((valid != 1))
    {
        print_usage();
        return -1;
    }
    
    return 0;
}

/**
    @fn         int main(int argc, char *argv[])
    @brief      ���ڲ��Ժ���
    @author     nick.xu
    @param[in]  argc        int         ��������
    @param[in]  argv        char**      ����ָ������
    @retval     0 �ɹ�
    @retval     -1 ʧ��
    @note       ��������ģʽ�ֱ���÷��ͺͽ��պ�����
*/
int main(int argc, char *argv[])
{
    int ret = 0;
    Para_t para;

    /* Ĭ�ϲ��� */
    memset(&para, 0x00, sizeof(Para_t));
    para.baud = 115200;
    para.number = 256;
    switch (para.baud)
    {
        case 9600:
            para.baud2 = B9600;
            break;
        case 19200:
            para.baud2 = B19200;
            break;
        case 38400:
            para.baud2 = B38400;
            break;
        case 57600:
            para.baud2 = B57600;
            break;
        case 115200:
        default:
            para.baud2 = B115200;
    }
    
    /* �������� */
    ret = parse_usage(argc, argv, &para);
    if (ret != 0)
    {
        ret = -1;
        goto Exit;
    }
    
    /* ��ӡ�����Ĳ��� */
    printf("%s %s baud=%d number=%d 8bit %s\n", s_string[para.mode], para.path, 
            para.baud, para.number, s_string2[para.check]);

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
    @brief      ���ʹ�������
    @author     nick.xu
    @param[in]  pPara       Para_t      �ڲ������ṹ��
    @retval     0 �ɹ�
    @retval     -1 ʧ��
    @note       ���������ô�������, Ȼ��������.
*/
static int send_data(Para_t *pPara)
{
    int fd = 0;
    int i = 0;
    int ret = 0;
    int sent = 0;
    unsigned char buffer[1024] = {0};
    struct termios option;
    int ctrlbits = 0;

    /* ���Է�������0x00 - 0xFF */
    for (i = 0; i < sizeof(buffer); i++)
    {
        buffer[i] = i;
    }

    /* �򿪷��ʹ��� */
    fd = open(pPara->path, O_RDWR, 0);
    if (fd == -1)
    {
        printf("open %s failed!%d\n", pPara->path, errno);
        return -21;
    }

    /* ��ȡ��ǰ���� */
    ret = tcgetattr(fd, &option);

    /* ���ò����� */
    ret = cfsetispeed(&option, pPara->baud2);
    ret = cfsetospeed(&option, pPara->baud2);

    /* ����λ8λ ֹͣλ1λ ��У�� */
    option.c_cflag |= (CLOCAL | CREAD);
    option.c_cflag |= PARENB;
    option.c_cflag |= PARODD;
    option.c_cflag &= ~CSTOPB;
    option.c_cflag &= ~CSIZE;
    option.c_cflag |= CS8;
    option.c_cflag &= ~CRTSCTS; /* ȡ��Ӳ�������� */

    /* ԭʼģʽ */
    option.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    option.c_iflag &= ~(IXON | IXOFF | IXANY | INLCR | ICRNL | IGNCR);
    option.c_oflag &= ~(OPOST | ONLCR | OCRNL);
    option.c_cc[VTIME] = 0;
    option.c_cc[VMIN] = 1;

    /* ����ģʽ����ջ����� */
    ret = tcsetattr(fd, TCSAFLUSH, &option);

    /* 422ģʽ��������RTS���ܷ��� */
    ctrlbits = TIOCM_RTS;
    ret = ioctl(fd, TIOCMBIC, &ctrlbits);

    /* ���ʹ��ڷ������� */
    sent = write(fd, buffer, pPara->number);
    if (sent != pPara->number)
    {
        printf("write failed!%d\n", errno);
        ret = -22;
        goto Exit;
    }

Exit:
    /* �رմ��� */
    if (fd != -1)
    {
        close(fd);
    }

    return ret;
}

/**
    @fn         static int receive_data(Para_t *pPara)
    @brief      ���մ�������
    @author     nick.xu
    @param[in]  pPara       Para_t      �ڲ������ṹ��
    @retval     0 �ɹ�
    @retval     -1 ʧ��
    @note       ���������ô�������, Ȼ��������������.
*/
static int receive_data(Para_t *pPara)
{
    int ret = 0;
    int fd = 0;
    unsigned char buffer[1024] = {0};
    int received = 0;
    int sum = 0;
    int i = 0;
    struct termios option;

    /* �򿪽��մ��� */
    fd = open(pPara->path, O_RDWR | O_NOCTTY);
    if (fd == -1)
    {
        printf("open %s failed!%d\n", pPara->path, errno);
        return -21;
    }

    /* ��ȡ��ǰ���� */
    ret = tcgetattr(fd, &option);

    /* ���ò����� */
    ret = cfsetispeed(&option, pPara->baud2);
    ret = cfsetospeed(&option, pPara->baud2);

    /* ����λ8λ ֹͣλ1λ ��У�� */
    option.c_cflag |= (CLOCAL | CREAD);
    option.c_cflag |= PARENB;
    option.c_cflag |= PARODD;
    option.c_cflag &= ~CSTOPB;
    option.c_cflag &= ~CSIZE;
    option.c_cflag |= CS8;
    option.c_cflag &= ~CRTSCTS; /* ȡ��Ӳ�������� */

    /* ԭʼģʽ */
    option.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    option.c_iflag &= ~(IXON | IXOFF | IXANY | INLCR | ICRNL | IGNCR);
    option.c_oflag &= ~(OPOST | ONLCR | OCRNL);
    option.c_cc[VTIME] = 10;    /* 10��֮1��Ϊ��λ */
    option.c_cc[VMIN] = 8;      /* ����X���������� */

    /* ����ģʽ����ջ����� */
    tcsetattr(fd, TCSAFLUSH, &option);

    /* ��ӡ�������� */
    printf("press any key to quit.\n");
    for (; ; )
    {
        ret = getch2();
        if (ret != -1)
        {
            break;
        }
        
        received = read(fd, buffer, sizeof(buffer));
        if (received == 0)
        {
            printf("read return 0!\n");
        }
        if (received == -1)
        {
            printf("read failed!%d\n", errno);
            ret = -21;
            goto Exit;
        }

        sum += received;
        printf("--- ");
        for (i = 0; i < received; i++)
        {
            printf("%02X ", buffer[i]);
            if (((i + 1) % 16) == 0) printf("\n    "); /* 16��һ���� */
        }
        printf("--- %s\n", pPara->name);
    }

    ret = 0;
    
Exit:
    /* �رմ��� */
    if (fd != -1)
    {
        close(fd);
    }

    return ret;
}
