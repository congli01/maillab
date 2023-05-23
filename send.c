#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "base64_utils.h"

#define MAX_SIZE 4095

char buf[MAX_SIZE+1];

/**
 * @brief
 * 接收服务器的数据并在终端用绿色字体打印出来
 * 
 * @param s_fd  套接字文件描述符
 * @param buf   缓冲区数组
 * @return      实际接收的数据字节数
*/
int recv_print(int s_fd, char buf[])
{
    int r_size = -1;
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("\033[1;32m%s\033[0m", buf);
    return r_size;
}

/**
 * @brief
 * 向服务器发送数据并将发送的数据在终端用蓝色字体打印出来
 * 
 * @param s_fd  套接字文件描述符
 * @param buf   缓冲区指针
 * @param len   发送数据的长度
 * @return      实际发送的数据字节数
*/
int send_print(int s_fd, void *buf, int len)
{
    int s_size = -1;
    printf("\033[1;34m%s\n\033[0m", buf);
    if ((s_size = send(s_fd, buf, len, 0)) == -1)
    {
        perror("send");
        exit(EXIT_FAILURE);
    }
    return s_size;
}

/**
 * @brief
 * 读取指定路径文件的全部内容
*/
char *read_file(const char* path)
{
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        perror("open file");
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END); // 指向文件尾
    int len = ftell(fp);
    fseek(fp, 0, SEEK_SET); // 指向文件头
    char *content = (char *)malloc(len);
    fread(content, 1, len, fp);
    fclose(fp);
    return content;
}

// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char* receiver, const char* subject, const char* msg, const char* att_path)
{
    const char* end_msg = "\r\n.\r\n";
    const char* host_name = "smtp.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 25; // SMTP server port
    const char* user = "xxxxxxxxxx@qq.com"; // TODO: Specify the user
    const char* pass = "xxxxxxxxxxxxxxxx"; // TODO: Specify the password
    const char* from = "xxxxxxxxxx@qq.com"; // TODO: Specify the mail address of the sender
    char dest_ip[16]; // Mail server IP address
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    int r_size;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **) host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i-1]));

    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the mail server
    // 创建套接字
    if ((s_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    // 建立TCP连接
    struct sockaddr_in *servaddr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));    // 服务器的地址和端口
    servaddr->sin_family = AF_INET;
    servaddr->sin_port = htons(port);   // 大小端转换
    servaddr->sin_addr = (struct in_addr){inet_addr(dest_ip)}; // 将char类型的dest_ip转换成32位二进制网络字节序的IPv4地址
    bzero(servaddr->sin_zero, 8);
    if (connect(s_fd, (struct sockaddr *)servaddr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    // 接收服务器的响应并输出到终端
    recv_print(s_fd, buf);

    // Send EHLO command and print server response
    const char* EHLO = "EHLO qq.com\r\n"; // TODO: Enter EHLO command here
    send_print(s_fd, (void *)EHLO, strlen(EHLO));

    // TODO: Print server response to EHLO command
    recv_print(s_fd, buf);
    
    // TODO: Authentication. Server response should be printed out.
    // 发送登录命令
    const char* AUTH = "AUTH login\r\n";    
    send_print(s_fd, (void *)AUTH, strlen(AUTH));
    recv_print(s_fd, buf);

    // 发送邮箱名，需base64编码
    char* user64 = encode_str(user);  // 邮箱名需base64编码
    strcat(user64, "\r\n");
    send_print(s_fd, (void *)user64, strlen(user64));
    recv_print(s_fd, buf);
    free(user64);

    // 发送邮箱授权码，需base64编码
    char* pass64 = encode_str(pass);
    strcat(pass64, "\r\n");
    send_print(s_fd, (void *)pass64, strlen(pass64));
    recv_print(s_fd, buf);
    free(pass64);

    // TODO: Send MAIL FROM command and print server response
    sprintf(buf, "MAIL FROM:<%s>\r\n", from);   // 字符串拼接
    send_print(s_fd, (void *)buf, strlen(buf));
    recv_print(s_fd, buf);
    
    // TODO: Send RCPT TO command and print server response
    sprintf(buf, "RCPT TO:<%s>\r\n", receiver);
    send_print(s_fd, (void *)buf, strlen(buf));
    recv_print(s_fd, buf);
    
    // TODO: Send DATA command and print server response
    const char* DATA = "data\r\n";
    send_print(s_fd, (void *)DATA, strlen(DATA));
    recv_print(s_fd, buf);

    // TODO: Send message data
    // 头部
    sprintf(buf, "From: %s\r\nTo: %s\r\nMIME_VERSION: 1.0\r\nContent-Type: multipart/mixed; boundary=qwertyuiopasdfghjklzxcvbnm\r\n", from, receiver);
    if (subject)
    {
        strcat(buf, "Subject: ");
        strcat(buf, subject);
        strcat(buf, "\r\n");
    }
    strcat(buf, "\r\n");    // 头部与消息体之间的空白行
    send_print(s_fd, (void *)buf, strlen(buf));

    // 消息体
    if (msg)
    {
        sprintf(buf, "--qwertyuiopasdfghjklzxcvbnm\r\nContent-Type:text/plain\r\n\r\n");    // 边界符
        send_print(s_fd, (void *)buf, strlen(buf));
        // 检查消息是否为文件
        if(access(msg, F_OK) == 0)
        {
            char *msg_content = read_file(msg);
            send(s_fd, msg_content, strlen(msg_content), 0);
            printf("send msg ...\n");
            free(msg_content);
        }
        else
        {
            send(s_fd, msg, strlen(msg), 0);
            printf("send msg ...\n");
        }
        send_print(s_fd, "\r\n", 2);
    }

    // 附件
    if (att_path)
    {
        sprintf(buf, "--qwertyuiopasdfghjklzxcvbnm\r\n"
        "Content-Type:application/octet-stream\r\n"
        "Content-Transfer-Encoding: base64\r\n"
        "Content-Disposition: attachment; filename=%s\r\n\r\n", att_path);
        send_print(s_fd, (void *)buf, strlen(buf));
        FILE *att_file = fopen(att_path, "r");
        if (att_file == NULL)
        {
            perror("open attchment");
            exit(EXIT_FAILURE);
        }
        FILE *att_file64 = fopen("tmp_file", "w");
        encode_file(att_file, att_file64);
        fclose(att_file); 
        fclose(att_file64);
        char *att_content = read_file("tmp_file");
        send(s_fd, att_content, strlen(att_content), 0);
        printf("send attchment ...\n");
    }
    sprintf(buf, "--qwertyuiopasdfghjklzxcvbnm\r\n");
    send_print(s_fd, (void *)buf, strlen(buf));

    // TODO: Message ends with a single period
    send_print(s_fd, (void *)end_msg, strlen(end_msg));
    recv_print(s_fd, buf);

    // TODO: Send QUIT command and print server response
    const char* QUIT = "quit\r\n";
    send_print(s_fd, (void *)QUIT, strlen(QUIT));
    recv_print(s_fd, buf);

    close(s_fd);
}

int main(int argc, char* argv[])
{
    int opt;
    char* s_arg = NULL;
    char* m_arg = NULL;
    char* a_arg = NULL;
    char* recipient = NULL;
    const char* optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }
}
