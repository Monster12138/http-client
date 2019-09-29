#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define MAX_URL_LEN          1024
#define SEND_BUFF_SIZE       2048
#define RECV_BUFF_SIZE       20480


//从标准输入接收并校验用户输入的URL
int get_url(char *url) {
    printf("Please input url: \n");
    scanf("%s", url);

    if(strlen(url) > MAX_URL_LEN) {
        printf("url is too long, please input url again!\n");
        return -1;
    }

    return 0;
}

//将URL中的域名和资源定位符分离
void get_hostname(char *url, char *host_name, char **path) {
    char   *start, *end;

    //检查用户输入的URL是否有HTTP://，如果有则忽略这一部分
    start = strstr(url, "http://");
    if(start == NULL) {
        start = url;

    } else {
        start = url + strlen("http://");
    }

    //检查是否有资源定位符，如果没有则置为默认值"/"
    end = strchr(start, '/');
    if(end == NULL) {
        memcpy(host_name, start, strlen(start));
        *path = "/";

    } else {
        memcpy(host_name, start, end - start);
        *path = end;
    }
    printf("[debug] Host: %s\n", host_name);
    printf("[debug] path: %s\n", *path);
}

//使用gethostbyname函数，通过域名获取服务器的IP地址
int get_ip(char *host_name, char *ip) {
    struct hostent    *ht;
    socklen_t          len;

    //系统函数，详见man gethostbyname
    ht = gethostbyname(host_name);
    if(ht == NULL || ht->h_addr == NULL) {
        printf("DNS resolution failed, unkown domain name!\n");
        return -1;
    }

    len = sizeof(struct sockaddr_in);
    inet_ntop(AF_INET, ht->h_addr, ip, len);
    printf("[debug] IP: %s\n", ip);

    return 0;
}

//通过获取到的IP地址，建立到服务器80端口的TCP连接,
//连接建立成功返回对应的套接字，否则返回-1
int connect_to_serv(char *ip, int port) {
    int                    ret, sockfd;
    socklen_t              addr_len;
    struct sockaddr_in     peer_addr;

    //创建套接字
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        printf("[error] socket error!\n");
        exit(0);
    }

    //初始化sockaddr_in结构体
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_addr.s_addr = inet_addr(ip);
    peer_addr.sin_port = htons(80);

    addr_len = sizeof(peer_addr);

    //发起连接
    printf("[debug] connecting to %s:80...\n", ip);
    ret = connect(sockfd, (struct sockaddr*)&peer_addr, addr_len);
    if(ret < 0) {
        printf("[notice] connect to server failed\n");
        return -1;
    }
    printf("[debug] connect success\n");

    return sockfd;
}

//构造HTTP报文并发送
int http_send(int sockfd, char *host_name, char *path, char *send_buf) {
    int        ret;
    
    //构造HTTP报文
    //
    //GET /yyy HTTP/1.1
    //Host: www.xxx.com
    //Accept: */*
    //
    sprintf(send_buf, 
            "GET %s HTTP/1.1\r\nHost: %s\r\nAccept: */*\r\n\r\n",
            path, host_name);

    //发送构造好的HTTP报文
    ret = send(sockfd, send_buf, strlen(send_buf), 0);
    if(ret < 0) {
        printf("[error] send error!\n");
        return -1;
    }
    printf("[debug] send %d bytes\n%s\n", ret, send_buf);

    return 0;
}

//接收服务器返回的数据并打印
int http_recv(int sockfd, char *recv_buf) {
    int        ret;

    ret = recv(sockfd, recv_buf, RECV_BUFF_SIZE, 0);
    if(ret < 0) {
        printf("[error] recv error!");
        return -1;
    } else if(ret == 0) {
        printf("[debug] connection is closed\n");
        return -1;
    } else {
        printf("[debug] recv %d bytes\n%s\n", ret, recv_buf);
        return 0;
    }
}

int main()
{
    while(1) {
        int     ret, sockfd;
        char   *path;
        char    url[MAX_URL_LEN + 2] = {0};
        char    ip[INET_ADDRSTRLEN + 1] = {0};
        char    host_name[MAX_URL_LEN] = {0};
        char    send_buf[SEND_BUFF_SIZE] = {0};
        char    recv_buf[RECV_BUFF_SIZE] = {0};

        ret = get_url(url);
        if(ret < 0) {
            continue;
        }

        get_hostname(url, host_name, &path);
        
        ret = get_ip(host_name, ip);
        if(ret < 0) {
            continue;
        }

        sockfd = connect_to_serv(ip, 80);
        if(sockfd < 0) {
            continue;
        }
        
        ret = http_send(sockfd, host_name, path, send_buf);
        if(ret < 0) {
            close(sockfd);
            continue;
        }

        ret = http_recv(sockfd, recv_buf);
        close(sockfd);
    }
    return 0;
}

