#ifndef TIME_WHEEL_TIMER
#define TIME_WHEEL_TIMER

#include <time.h>
#include <netinet/in.h>
#include <stdio.h>

#define BUFFER_SIZE 64
class tw_timer;
//绑定socket和定时器
struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[ BUFFER_SIZE ];
    tw_timer* timer;
};

//定时器类
class tw_timer
{
public:
    tw_timer( int rot, int ts ) 
    : next( NULL ), prev( NULL ), rotation( rot ), time_slot( ts ){}

public:
    //记录定时器在时间轮转多少圈后生效
    int rotation;
    //记录定时器在时间轮上属于哪个槽
    int time_slot;
    //定时器回调函数
    void (*cb_func)( client_data* );
    //客户数据
    client_data* user_data;
    //指向后一个定时器
    tw_timer* next;
    //指向前一个定时器
    tw_timer* prev;
};

class time_wheel
{
public:
    time_wheel() : cur_slot( 0 )
    {
        for( int i = 0; i < N; ++i )
        {
            //初始化每个槽的头结点
            slots[i] = NULL;
        }
    }

    ~time_wheel()
    {
        //遍历每个槽，并且销毁其中所有的定时器
        for( int i = 0; i < N; ++i )
        {
            tw_timer* tmp = slots[i];
            while( tmp )
            {
                slots[i] = tmp->next;
                delete tmp;
                tmp = slots[i];
            }
        }
    }
    //根据定时值timeout创建一个定时器，并将它插入到合适的槽中
    tw_timer* add_timer( int timeout )
    {
        if( timeout < 0 )
        {
            return NULL;
        }
        int ticks = 0;
        if( timeout < TI )
        {
            ticks = 1;
        }
        else
        {
            ticks = timeout / TI;
        }
        //计算待插入的定时器在时间轮转动多少圈后被触发
        int rotation = ticks / N;
        //计算待插入的定时器应该被插入到哪个槽中
        int ts = ( cur_slot + ( ticks % N ) ) % N;
        //创建新的定时器
        tw_timer* timer = new tw_timer( rotation, ts );
        //如果第ts个槽中尚无定时器，则把新建的定时器插入其中，并将该定时器设为该槽的头结点
        if( !slots[ts] )
        {
            printf( "add timer, rotation is %d, ts is %d, cur_slot is %d\n", rotation, ts, cur_slot );
            slots[ts] = timer;
        }
        //否则，将定时器插入第ts个槽中
        else
        {
            timer->next = slots[ts];
            slots[ts]->prev = timer;
            slots[ts] = timer;
        }
        return timer;
    }
    //删除目标 定时器timer
    void del_timer( tw_timer* timer )
    {
        if( !timer )
        {
            return;
        }
        int ts = timer->time_slot;
        //slots[ts]是目标定时器所在槽的头结点
        if( timer == slots[ts] )
        {
            slots[ts] = slots[ts]->next;
            if( slots[ts] )
            {
                slots[ts]->prev = NULL;
            }
            delete timer;
        }
        else
        {
            timer->prev->next = timer->next;
            if( timer->next )
            {
                timer->next->prev = timer->prev;
            }
            delete timer;
        }
    }
    //SI时间到后，调用该函数，时间轮向前滚动一个槽的间隔
    void tick()
    {
        //取得时间轮上当前槽的头结点
        tw_timer* tmp = slots[cur_slot];
        printf( "current slot is %d\n", cur_slot );
        while( tmp )
        {
            printf( "tick the timer once\n" );
            //如果定时器的rotation的值大于0，则在这一轮中不起作用
            if( tmp->rotation > 0 )
            {
                tmp->rotation--;
                tmp = tmp->next;
            }
            //否则，说明定时器已到达，执行定时任务，并删除该定时器
            else
            {
                tmp->cb_func( tmp->user_data );
                if( tmp == slots[cur_slot] )
                {
                    printf( "delete header in cur_slot\n" );
                    slots[cur_slot] = tmp->next;
                    delete tmp;
                    if( slots[cur_slot] )
                    {
                        slots[cur_slot]->prev = NULL;
                    }
                    tmp = slots[cur_slot];
                }
                else
                {
                    tmp->prev->next = tmp->next;
                    if( tmp->next )
                    {
                        tmp->next->prev = tmp->prev;
                    }
                    tw_timer* tmp2 = tmp->next;
                    delete tmp;
                    tmp = tmp2;
                }
            }
        }
        //更新时间轮的当前槽，以反映时间轮的转动
        cur_slot = ++cur_slot % N;
    }

private:
    //时间轮上槽的数目
    static const int N = 60;
    //槽间隔1s
    static const int TI = 1;
    //时间轮的槽，其中每个元素指向一个定时器链表，链表无序 
    tw_timer* slots[N];
    //时间轮的当前槽
    int cur_slot;
};

#endif
