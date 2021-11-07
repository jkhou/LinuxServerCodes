#ifndef LST_TIMER
#define LST_TIMER

#include <time.h>

#define BUFFER_SIZE 64
//前向声明
class util_timer;
//用户数据结构：客户端socket地址、socket文件描述符、读缓存和定时器
struct client_data
{
    sockaddr_in address;
    int sockfd;
    char buf[ BUFFER_SIZE ];
    util_timer* timer;
};

//定时器类
class util_timer
{
public:
    util_timer() : prev( NULL ), next( NULL ){}

public:
    //任务的超时时间，这里使用绝对时间
   time_t expire; 
   //任务回调函数
   void (*cb_func)( client_data* );
   //回调函数处理的客户数据，由定时器的执行者传递给回调函数
   client_data* user_data;
   //指向前一个定时器
   util_timer* prev;
   //指向后一个定时器
   util_timer* next;
};

//定时器链表，是一个升序、双向链表，并且带有头结点和尾结点
class sort_timer_lst
{
public:
    sort_timer_lst() : head( NULL ), tail( NULL ) {}
    //链表被销毁时，删除其中所有的定时器
    ~sort_timer_lst()
    {
        util_timer* tmp = head;
        while( tmp )
        {
            head = tmp->next;
            delete tmp;
            tmp = head;
        }
    }
    //将目标定时器timer添加到链表上
    void add_timer( util_timer* timer )
    {
        if( !timer )
        {
            return;
        }
        if( !head )
        {
            head = tail = timer;
            return; 
        }
        //如果目标定时器的超时时间小于链表中所有定时器的超时时间，则把该定时器插入链表头部，作为链表新的头结点。
        if( timer->expire < head->expire )
        {
            timer->next = head;
            head->prev = timer;
            head = timer;
            return;
        }
        //否则，调用重载函数，将此定时器插入到链表合适的位置
        add_timer( timer, head );
    }
    //当某个定时任务发生变化时，调整对应的定时器在链表中的位置
    void adjust_timer( util_timer* timer )
    {
        if( !timer )
        {
            return;
        }
        util_timer* tmp = timer->next;
        //如果被调整的定时器在链表尾部，或者该定时器超时值小于笑一个定时器超时值，则无需调整
        if( !tmp || ( timer->expire < tmp->expire ) )
        {
            return;
        }
        //如果定时器是链表头结点，则将该定时器取出并重新插入链表中
        if( timer == head )
        {
            head = head->next;
            head->prev = NULL;
            timer->next = NULL;
            add_timer( timer, head );
        }
        //否则，将定时器从链表中取出，插入其原来所在位置后的部分链表中
        else
        {
            timer->prev->next = timer->next;
            timer->next->prev = timer->prev;
            add_timer( timer, timer->next );
        }
    }
    //将目标定时器从链表中删除
    void del_timer( util_timer* timer )
    {
        if( !timer )
        {
            return;
        }
        //如果链表中只有一个定时器，即目标定时器
        if( ( timer == head ) && ( timer == tail ) )
        {
            delete timer;
            head = NULL;
            tail = NULL;
            return;
        }
        //如果目标定时器是链表的头结点，则将头结点重置为原头结点的下一个结点，并删除目标定时器
        if( timer == head )
        {
            head = head->next;
            head->prev = NULL;
            delete timer;
            return;
        }
        //如果目标定时器是链表的尾结点，则将尾结点重置为原尾结点的上一个结点，并删除目标定时器
        if( timer == tail )
        {
            tail = tail->prev;
            tail->next = NULL;
            delete timer;
            return;
        }
        //如果目标定时器在链表的中间位置，则删除该定时器
        timer->prev->next = timer->next;
        timer->next->prev = timer->prev;
        delete timer;
    }
    //SIGALRM信号每次被触发就在其信号处理函数中执行一次tick函数，以 处理链表上到期的任务
    void tick()
    {
        if( !head )
        {
            return;
        }
        printf( "timer tick\n" );
        time_t cur = time( NULL );
        util_timer* tmp = head;
        //从头结点开始依次处理每个定时器，直到遇到一个尚未到期的定时器
        while( tmp )
        {
            if( cur < tmp->expire )
            {
                break;
            }
            //调用定时器的回调函数，以执行定时任务
            tmp->cb_func( tmp->user_data );
            //执行完定时器中的定时任务之后，就将它从链表中删除，并重置链表头结点
            head = tmp->next;
            if( head )
            {
                head->prev = NULL;
            }
            delete tmp;
            tmp = head;
        }
    }

private:
    //重载函数，表示将目标定时器timer添加到节点lst_head之后的部分链表中
    void add_timer( util_timer* timer, util_timer* lst_head )
    {
        util_timer* prev = lst_head;
        util_timer* tmp = prev->next;
        //遍历lst_head之后的部分链表，如果找到一个超时时间大于目标定时器超时时间的节点，则将定时器插入该节点之前
        while( tmp )
        {
            if( timer->expire < tmp->expire )
            {
                prev->next = timer;
                timer->next = tmp;
                tmp->prev = timer;
                timer->prev = prev;
                break;
            }
            prev = tmp;
            tmp = tmp->next;
        }
        //否则，将该定时器插入链表尾部，并将它设置为链表新的尾结点
        if( !tmp )
        {
            prev->next = timer;
            timer->prev = prev;
            timer->next = NULL;
            tail = timer;
        }
        
    }

private:
    util_timer* head;
    util_timer* tail;
};

#endif
