#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_SIZE 65535

char buf[MAX_SIZE+1];

/**
 * @brief
 * 接收服务器的数据并在终端用绿色字体打印出来
 * 
 * @param s_fd  套接字文件描述符
 * @param buf   缓冲区数组
 * @param flag  0：打印，1：不打印
 * @return      实际接收的数据字节数
*/
int recv_print(int s_fd, char buf[], int flag)
{
    int r_size = -1;
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    if(flag)
    {
        printf("\033[1;32m%s\033[0m", buf);
    }
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

void recv_mail()
{
    const char* host_name = "pop.qq.com"; // TODO: Specify the mail server domain name
    const unsigned short port = 110; // POP3 server port
    const char* user = "xxxxxxxxxx@qq.com"; // TODO: Specify the user
    const char* pass = "xxxxxxxxxxxxxxxx"; // TODO: Specify the password
    char dest_ip[16];
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

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
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
    recv_print(s_fd, buf, 1);

    // TODO: Send user and password and print server response
    // 邮箱和授权码不需要base64编码
    sprintf(buf, "user %s\r\n", user);
    send_print(s_fd, (void *)buf, strlen(buf));
    recv_print(s_fd, buf, 1);
    sprintf(buf, "pass %s\r\n", pass);
    send_print(s_fd, (void *)buf, strlen(buf));
    recv_print(s_fd, buf, 1);

    // TODO: Send STAT command and print server response
    const char *STAT = "stat\r\n";
    send_print(s_fd, (void *)STAT, strlen(STAT));
    recv_print(s_fd, buf, 1);

    // TODO: Send LIST command and print server response
    const char *LIST = "list\r\n";
    send_print(s_fd, (void *)LIST, strlen(LIST));
    recv_print(s_fd, buf, 1);

    // TODO: Retrieve the first mail and print its content
    const char *RETR = "retr 1\r\n";
    send_print(s_fd, (void *)RETR, strlen(RETR));
    r_size = recv_print(s_fd, buf, 1);
    int remain_size = atoi(buf + 4);
    remain_size -= r_size;
    while(remain_size > 0)
    {
        // 后面的邮件内容没有输出到终端了，若要输出可将参数改为1
        r_size = recv_print(s_fd, buf, 0); 
        printf(".\n");
        remain_size -= r_size;
    }

    // TODO: Send QUIT command and print server response
    const char* QUIT = "quit\r\n";
    send_print(s_fd, (void *)QUIT, strlen(QUIT));
    recv_print(s_fd, buf, 1);

    close(s_fd);
}

int main(int argc, char* argv[])
{
    recv_mail();
    exit(0);
}
