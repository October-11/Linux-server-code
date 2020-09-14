#include <sys/socket.h>
#include <winsock.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet/h>
#include <assert.h>
#include <stdlib.h>
#include <poll.h>
#include <fcntl.h>

#define BUFFER_SIZE 64

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));                //调用basename函数(头文件为libgen.h)解析argv[0]
        return 1;
    }
    const char* ip = argv[0];
    int port = atoi(argv[1]);

    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.S_addr = inet_addr(ip);
    //inet_pton(AF_INET, ip, &server_address.sin_addr);

    int sock_client = socket(PF_INET, SOCK_STREAM, 0);
    if (connect(sock_client, (struct sockaddr_in*)&server_address, sizeof(server_address)) < 0) {
        printf("connection failed\n");
        close(sock_client);
        return 1;
    }

    pollfd fds[2];
    fds[0].fd = 0;                          //标准输入
    fds[0].events = POLLIN;
    fds[0].revents = 0;
    fds[1].fd = sock_client;                //客户端套接字
    fds[1].events = POLLIN | POLLRDHUP;
    fds[1].revents = 0;

    char read_buf[BUFFER_SIZE];
    int pipefd[2];
    int ret = pipe(pipefd);
    assert(ret != -1);                      //assert:如果表达式结果为false，则终止程序运行

    while (1) {
        ret = poll(fds, 2, -1);
        if (ret < 0) {
            printf("poll failed\n");
            break;
        }
        if (fds[1].revents & POLLRDHUB) {
            printf("server close the connection\n");
            return 1;
        }
        else if (fds[1].revents & POLLIN) {
            memset(read_buf, '\0', BUFFER_SIZE)；
            recv(fds[1].fd, read_buf, BUFFER_SIZE - 1, 0);
            printf("%s\n", read_buf);
        }
        if (fds[0].revents & POLLIN) {
            ret = splice(0, NULL, pipefd[1], NULL, 32768, SPlICE_F_MORE | SPLICE_F_MOVE);
            ret = splice(pipefd[0], NULL, sock_client, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
        }
    }
    close(sock_client);
    return 0;
}