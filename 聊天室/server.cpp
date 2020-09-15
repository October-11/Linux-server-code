#define _GNU_SOURCE 1
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

#define USER_LIMIT 5
#define BUFFER_SIZE 64
#define FD_LIMIT 65535

struct client_data
{
    sockaddr_in address;
    char* write_buf;
    char buf[BUFFER_SIZE];
}

//将文件描述符设置成非阻塞的
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("usage: %s ip_adress port_number\n", basename(argv[0]));
        return 1;
    }
    const char* ip = argv[1];
    const port = argv[0];

    int ret = 0;                                                                //定义通信信息地址
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, ip, &address.sin_addr);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);                             //创建socket
    assert(listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));       //绑定端口号和地主
    assert(ret != -1);

    ret = listen(listenfd, 5);                                                  //设置监听
    assert(ret != -1);

    client_data* users = new client_data[FD_LIMIT];
    pollfd fds[USER_LIMIT];
    int user_counter = 0;
    for (int i = 1; i < USER_LIMIT; i++) {
        fds[i].fd = -1;
        fds[i].events = 0;
    }
    //设置监听的I/O复用
    fds[0].fd = listenfd;
    fds[0].events = POLLIN | POLLERR;
    fds[0].revents = 0;

    while (1) {
        ret = poll(fds, user_counter + 1, -1);
        if (ret < 0) {
            printf("poll failure\n");
            break;
        }
        for (int i = 0; i < user_counter + 1; i++) {
            //如果新来了一个用户,建立socket连接
            if ((fds[i].revents & POLLIN) && (fds[i].fd == listenfd)) {                 
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
                if (connfd < 0) {
                    printf("errno is: %d\n", errno)
                    continue;
                }
                if (user_counter >= USER_LIMIT) {
                    const char* info = "too many users\n"
                    printf("%s", info);
                    send(connfd, info, strlen(info), 0);
                    close(connfd);
                    continue;
                }
                //对于新的连接，同时修改fds和users数组，users[connfd]对应于新连接文件描述符connfd的客户端数据
                user_counter++;
                users[connfd].address = client_address;
                setnonblocking(connfd);
                fds[user_counter].fd = connfd;
                fds[user_counter].events = POLLIN | POLLRDHUB | POLLERR;
                fds[user_counter].revents = 0;
                printf("comes a new user, now have %d user\n", user_counter);
            }
            //如果出现错误,则报告在第几个用户出现错误
            else if (fds[i].revents & POLLERR) {
                printf("get an error from %d\n", fds[i].fd);
                char errors[100];
                memset(errors, '\0', 100);
                socklen_t length = sizeof(errors);
                if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0) {
                    printf("get socket option failed\n");
                }
                continue;
            }
            else if (fds[i].revents & POLLRDHUB) {
                //如果客户端关闭连接，则服务器也关闭连接，并将用户数减一
                users[fds[i].fd] = users[fds[user_counter].fd];
                close(fds[i].fd);
                fds[i] = fds[user_counter];
                i--;
                user_counter--;
                printf("a client left\n");
            }
            else if (fds[i].revents & POLLIN) {
                //如果一个客户端需要读取数据
                int connfd = fds[i].fd;
                memset(users[connfd].buf, '\0', BUFFER_SIZE);
                //从服务器接收套接字接收数据到缓冲区
                ret = recv(connfd, users[connfd].buf, BUFFER_SIZE - 1, 0);
                printf("get %s bytes of client data %s from %d\n", ret, users[connfd].buf, connfd);
                if (ret < 0) {
                    //如果读操作出错，则关闭连接
                    if (errno != EAGAIN) {
                        close(connfd);
                        users[fds[i].fd] = users[fds[user_counter].fd];
                        fds[i] = fds[user_counter];
                        i--;
                        user_counter--;
                    }
                }
                else if (ret == 0) {

                }
                else {
                    //如果接收到客户数据，则通知其他socket准备写数据
                    for (int j = 1; j <= user_counter; j++) {
                        if (fds[j].fd = connfd) {
                            continue;
                        }
                        fds[j].events |= ~POLLIN;
                        fds[j].events |= ~POLLOUT;
                        users[fds[j].fd].write_buf = users[connfd].buf;
                    }
                }
            }
            else if (fds[i].revents & POLLOUT) {
                //存放需要写入操作的文件描述符
                int connfd = fds[i].fd;
                if (!users[connfd].write_buf) {
                    continue;
                }
                //将需要写的数据从客户端的写缓冲区发送到对应的socket描述符
                ret = send(connfd, users[connfd].write_buf, strlen(users[connfd].write_buf), 0);
                users[connfd].write_buf = NULL;
                //写完数据后需要重新注册fds[i]上的可读事件
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }
    delete [] users;
    close(listenfd);
    return 0;
}