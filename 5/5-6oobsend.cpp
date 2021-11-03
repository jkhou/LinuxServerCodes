#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

int main( int argc, char* argv[] )
{
    if( argc <= 2 )
    {
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );

    struct sockaddr_in server_address;
    bzero( &server_address, sizeof( server_address ) );
    server_address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &server_address.sin_addr );
    server_address.sin_port = htons( port );

    int sockfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( sockfd >= 0 );
    //客户端通过connent系统调用主动与服务器建立连接，成功时返回0
    if ( connect( sockfd, ( struct sockaddr* )&server_address, sizeof( server_address ) ) < 0 )
    {
        printf( "connection failed\n" );
    }
    else
    {
        printf( "send oob data out\n" );
        const char* oob_data = "abc";
        const char* normal_data = "123";
        //send往sockfd中写入数据，buf和len分别指明写缓冲区的位置和大小
        //send成功时返回实际写入数据的长度，失败时返回-1并且设置errno
        send( sockfd, normal_data, strlen( normal_data ), 0 );
        //发送端一次发送的多字节的带外数据只有最后一个字节被当做带外数据，其余字节被当作普通数据
        send( sockfd, oob_data, strlen( oob_data ), MSG_OOB );  //MSG_OOB表示发送紧急消息
        send( sockfd, normal_data, strlen( normal_data ), 0 );
    }

    close( sockfd );
    return 0;
}
