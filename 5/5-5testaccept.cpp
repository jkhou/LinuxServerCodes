#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

int main( int argc, char* argv[] )
{
    if( argc <= 2 )
    {
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    int sock = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sock >= 0 );

    int ret = bind( sock, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );

    ret = listen( sock, 5 );
    assert( ret != -1 );

    //暂停10s以等待客户端连接和相关操作（掉线或退出）完成
    sleep(10);
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof( client );
    //连接成功时返回一个新的连接socket，服务器可通过该socket与被连接的客户端通信
    int connfd = accept( sock, ( struct sockaddr* )&client, &client_addrlength );
    //accept只是从监听队列中取出连接，而不管连接处于何种状态
    if ( connfd < 0 )
    {
        printf( "errno is: %d\n", errno );
    }
    else
    {
        //连接成功则打印出客户端的IP地址和端口号
        char remote[INET_ADDRSTRLEN ];
        printf( "connected with ip: %s and port: %d\n", 
            inet_ntop( AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN ), //将用网络字节序表示的IP地址转换为用字符串表示的IP地址
            ntohs( client.sin_port ) //short类型的网络字节序转换为主机字节序端口号
            );
        close( connfd );
    }
    
    close( sock );
    return 0;
}

