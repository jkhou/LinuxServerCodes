#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

//超时连接函数
int timeout_connect( const char* ip, int port, int time )
{
    int ret = 0;
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );

    int sockfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sockfd >= 0 );

    //超时时间类型为timeval，和select系统调用的超时参数类型相同
    struct timeval timeout;
    timeout.tv_sec = time;
    timeout.tv_usec = 0;
    socklen_t len = sizeof( timeout );
    //设置sockfd发送数据超时时间
    ret = setsockopt( sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len );
    assert( ret != -1 );

    while(1){
        ret = connect( sockfd, ( struct sockaddr* )&address, sizeof( address ) );
        if ( ret == -1 )
        {
            //connect超时对应的错误号是EINPROGRESS
            if( errno == EINPROGRESS )
            {
                printf( "connecting timeout\n" );
                return -1;
            }
            printf( "error occur when connecting to server\n" );
            // return -1;
            sleep(1);
        }

    }

    return sockfd;
}

int main( int argc, char* argv[] )
{
    if( argc <= 2 )
    {
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    //调用超时连接函数，返回一个文件描述符，超时时间为6s
    int sockfd = timeout_connect( ip, port, 6 );
    if ( sockfd < 0 )
    {
        return 1;
    }
    return 0;
}
