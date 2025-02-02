#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

//定义缓冲区大小
#define BUFFER_SIZE 4096
//主状态机的两种可能状态：当前正在分析请求行，当前正在分析头部字段
enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
//从状态机的三种可能状态：读取到一个完整的行、行出错、行数据尚且不完整
enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };
//服务器处理HTTP请求的结果
enum HTTP_CODE { NO_REQUEST,  //请求不完整，需要继续读取客户数据
                GET_REQUEST,  //获得了一个完整的客户请求
                BAD_REQUEST,  //客户请求有语法错误
                FORBIDDEN_REQUEST, //客户对资源没有足够的访问权限
                INTERNAL_ERROR,    //服务器内部错误
                CLOSED_CONNECTION  //客户端已经关闭连接  
                };
//为了简化，只是根据服务器的处理结果发送成功或失败信息
static const char* szret[] = { "I get a correct result\n", "Something wrong\n" };

//从状态机，用于解析出一行的内容
LINE_STATUS parse_line( char* buffer, int& checked_index, int& read_index )
{
    char temp;
    //checked_index指向buffer中当前正在分析的字节
    //read_index指向buffer中客户数据尾部的下一字节
    for ( ; checked_index < read_index; ++checked_index )
    {
        //获取当前要分析的字节，存到temp中
        temp = buffer[ checked_index ];
        //如果当前的字节是‘\r’，即回车符，说明可能读取到一个完整的行
        if ( temp == '\r' )
        {
            //如果回车符是buffer中的最后一个字节，说明此次没有读取到一个完整的行，返回LINE_OPEN表示还需要继续读取客户数据
            if ( ( checked_index + 1 ) == read_index )
            {
                return LINE_OPEN;
            }
            //如果下一个字符是‘\n’，说明成功读取到了一个完整的行
            else if ( buffer[ checked_index + 1 ] == '\n' )
            {
                buffer[ checked_index++ ] = '\0';
                buffer[ checked_index++ ] = '\0';
                return LINE_OK;
            }
            //否则，说明HTTP请求存在语法错误
            return LINE_BAD;
        }
        //如果当前换行符为'\n'，也说明可能读取到一个完整的行
        else if( temp == '\n' )
        {
            //如果前一个字节是'\r',说明此次成功读取到了一个完整的行
            if( ( checked_index > 1 ) &&  buffer[ checked_index - 1 ] == '\r' )
            {
                buffer[ checked_index-1 ] = '\0';
                buffer[ checked_index++ ] = '\0';
                return LINE_OK;
            }
            //否则，说明HTTP请求存在语法错误
            return LINE_BAD;
        }
    }
    //如果所有buffer中的内容分析完都没有遇到‘\r’，则需要继续读取客户端数据，返回LINE_OPEN
    return LINE_OPEN;
}

//分析HTTP请求行
HTTP_CODE parse_requestline( char* szTemp, CHECK_STATE& checkstate )
{
    //检索szTemp中第一个匹配\t的字符，未找到则返回NULL
    char* szURL = strpbrk( szTemp, " \t" );
    //如果请求行中没有空白字符或'\t'，则说明HTTP请求存在语法错误
    if ( ! szURL )
    {
        return BAD_REQUEST;
    }
    *szURL++ = '\0';

    char* szMethod = szTemp;
    if ( strcasecmp( szMethod, "GET" ) == 0 )
    {
        printf( "The request method is GET\n" );
    }
    else
    {
        return BAD_REQUEST;
    }
    //检索szURL中第一个不为\t的字符下标
    szURL += strspn( szURL, " \t" );
    char* szVersion = strpbrk( szURL, " \t" );
    if ( ! szVersion )
    {
        return BAD_REQUEST;
    }

    *szVersion++ = '\0';
    szVersion += strspn( szVersion, " \t" );
    //仅支持HTTP1.1
    if ( strcasecmp( szVersion, "HTTP/1.1" ) != 0 )
    {
        return BAD_REQUEST;
    }

    //检查url是否合法
    if ( strncasecmp( szURL, "http://", 7 ) == 0 )
    {
        szURL += 7;
        szURL = strchr( szURL, '/' );
    }

    if ( ! szURL || szURL[ 0 ] != '/' )
    {
        return BAD_REQUEST;
    }

    //URLDecode( szURL );
    printf( "The request URL is: %s\n", szURL );
    //HTTP请求行处理完毕，状态转移至头部字段的分析
    checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//分析HTTP头部字段 
HTTP_CODE parse_headers( char* szTemp )
{
    //若遇到空行，则说明得到了一个完整的HTTP请求
    if ( szTemp[ 0 ] == '\0' )
    {
        return GET_REQUEST;
    }
    //处理host头部字段
    else if ( strncasecmp( szTemp, "Host:", 5 ) == 0 )
    {
        szTemp += 5;
        szTemp += strspn( szTemp, " \t" );
        printf( "the request host is: %s\n", szTemp );
    }
    //其他头部字段不处理
    else
    {
        printf( "I can not handle this header\n" );
    }
    //请求不完整，继续读取客户端数据
    return NO_REQUEST;
}

//分析HTTP请求的入口函数
HTTP_CODE parse_content( char* buffer, int& checked_index, CHECK_STATE& checkstate, int& read_index, int& start_line )
{
    //记录当前行的读取状态
    LINE_STATUS linestatus = LINE_OK;
    //记录HTTP请求的处理结果
    HTTP_CODE retcode = NO_REQUEST;
    //主状态机，用于从buffer中取出所有完整的行
    while( ( linestatus = parse_line( buffer, checked_index, read_index ) ) == LINE_OK )
    {
        //start_line是行在buffer中的起始位置
        char* szTemp = buffer + start_line;
        //记录下一行的起始位置
        start_line = checked_index;
        switch ( checkstate )
        {
            //第一个状态：分析请求行
            case CHECK_STATE_REQUESTLINE:
            {
                retcode = parse_requestline( szTemp, checkstate );
                //客户请求有语法错误
                if ( retcode == BAD_REQUEST )
                {
                    return BAD_REQUEST;
                }
                break;
            }
            //第二个状态：分析头部字段
            case CHECK_STATE_HEADER:
            {
                retcode = parse_headers( szTemp );
                //客户请求有语法错误
                if ( retcode == BAD_REQUEST )
                {
                    return BAD_REQUEST;
                }
                //获得了一个完整的客户请求
                else if ( retcode == GET_REQUEST )
                {
                    return GET_REQUEST;
                }
                break;
            }
            default:
            {
                //否则，返回服务器内部错误
                return INTERNAL_ERROR;
            }
        }
    }
    //如果没有读取到一个完整的行，则还需要继续读取客户数据
    if( linestatus == LINE_OPEN )
    {
        return NO_REQUEST;
    }
    else
    {
        return BAD_REQUEST;
    }
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
    
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );
    
    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );
    
    int ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );
    
    ret = listen( listenfd, 5 );
    assert( ret != -1 );
    
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof( client_address );
    int fd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
    if( fd < 0 )
    {
        printf( "errno is: %d\n", errno );
    }
    else
    {
        //创建读缓冲区
        char buffer[ BUFFER_SIZE ];
        memset( buffer, '\0', BUFFER_SIZE );
        int data_read = 0;
        //当前已经读取了多少字节的客户数据
        int read_index = 0;
        //当前已经分析了多少字节的客户数据
        int checked_index = 0;
        //行在buffer中的起始位置
        int start_line = 0;
        //设置主状态机的初始位置
        CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
        //循环读取客户数据并且分析
        while( 1 )
        {
            data_read = recv( fd, buffer + read_index, BUFFER_SIZE - read_index, 0 );
            if ( data_read == -1 )
            {
                printf( "reading failed\n" );
                break;
            }
            else if ( data_read == 0 )
            {
                printf( "remote client has closed the connection\n" );
                break;
            }
    
            read_index += data_read;
            //分析目前已经获得的所有客户数据
            HTTP_CODE result = parse_content( buffer, checked_index, checkstate, read_index, start_line );
            //尚未得到一个完整的HTTP请求
            if( result == NO_REQUEST )
            {
                continue;
            }
            //得到了一个完整的 正确的HTTP请求
            else if( result == GET_REQUEST )
            {
                send( fd, szret[0], strlen( szret[0] ), 0 );
                break;
            }
            //其他情况表示发生了错误
            else
            {
                send( fd, szret[1], strlen( szret[1] ), 0 );
                break;
            }
        }
        close( fd );
    }
    
    close( listenfd );
    return 0;
}
