#define TIMEOUT 5000

int timeout = TIMEOUT;
time_t start = time( NULL );
time_t end = time( NULL );
while( 1 )
{
    printf( "the timeout is now %d mill-seconds\n", timeout );
    start = time( NULL );
    int number = epoll_wait( epollfd, events, MAX_EVENT_NUMBER, timeout );
    if( ( number < 0 ) && ( errno != EINTR ) )
    {
        printf( "epoll failure\n" );
        break;
    }
    //如果epoll_wati返回0，则说明超时时间到了，处理定时任务，重置定时事件
    if( number == 0 )
    {
        // timeout
        timeout = TIMEOUT;
        continue;
    }

    end = time( NULL );
    //如果epoll_wait返回值大于0，则将timeout减去这段持续时间，以获得下次epoll_wait调用的超时参数
    timeout -= ( end - start ) * 1000;
    //处理定时任务，重置定时时间 
    if( timeout <= 0 )
    {
        // timeout
        timeout = TIMEOUT;
    }

    // handle connections
}
