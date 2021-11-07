#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
//线程同步机制的包装类
#include "locker.h"

//线程池类
template< typename T >
class threadpool
{
public:
    threadpool( int thread_number = 8, int max_requests = 10000 );
    ~threadpool();
    //往请求队列中添加任务
    bool append( T* request );

private:
    //工作线程运行的函数，它不断的从队列中取出任务并执行之
    static void* worker( void* arg );
    void run();

private:
    //线程池中线程的数量
    int m_thread_number;
    //请求队列中最多允许的、等待处理的请求的数量
    int m_max_requests;
    //描述线程池的数组，大小为m_thread_number
    pthread_t* m_threads;
    //请求队列
    std::list< T* > m_workqueue;
    //保护请求队列的互斥锁
    locker m_queuelocker;
    //是否有任务需要处理
    sem m_queuestat;
    //是否结束线程
    bool m_stop;
};

//线程池构造函数
template< typename T >
threadpool< T >::threadpool( int thread_number, int max_requests ) : 
        m_thread_number( thread_number ), m_max_requests( max_requests ), m_stop( false ), m_threads( NULL )
{
    if( ( thread_number <= 0 ) || ( max_requests <= 0 ) )
    {
        throw std::exception();
    }

    m_threads = new pthread_t[ m_thread_number ];
    if( ! m_threads )
    {
        throw std::exception();
    }

    //创建thread_number个线程
    for ( int i = 0; i < thread_number; ++i )
    {
        printf( "create the %dth thread\n", i );
        //创建一个线程，成功时返回0，失败时返回错误码。设置this指针
        if( pthread_create( m_threads + i, NULL, worker, this ) != 0 )
        {
            delete [] m_threads;
            throw std::exception();
        }
        //设置线程为脱离线程，脱离了进程中其他线程的同步，退出时将自动释放所占有的内存资源
        if( pthread_detach( m_threads[i] ) )
        {
            delete [] m_threads;
            throw std::exception();
        }
    }
}

//销毁线程池
template< typename T >
threadpool< T >::~threadpool()
{
    delete [] m_threads;
    m_stop = true;
}

//往请求队列中添加任务
template< typename T >
bool threadpool< T >::append( T* request )
{
    //操作工作队列时先加锁，因为工作队列被所有线程共享
    m_queuelocker.lock();
    //如果工作队列的大小已经超出限制，则解锁返回
    if ( m_workqueue.size() > m_max_requests )
    {
        m_queuelocker.unlock();
        return false;
    }
    m_workqueue.push_back( request );
    //插入工作队列后，解锁
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

//工作线程运行的函数，它不断的从队列中取出任务并执行之
template< typename T >
void* threadpool< T >::worker( void* arg )
{
    threadpool* pool = ( threadpool* )arg;
    //获取this指针并调用其动态方法run
    pool->run();
    return pool;
}

template< typename T >
void threadpool< T >::run()
{
    while ( ! m_stop )
    {
        //等待信号量
        m_queuestat.wait();
        //工作队列上锁
        m_queuelocker.lock();
        if ( m_workqueue.empty() )
        {
            m_queuelocker.unlock();
            continue;
        }
        //取出工作队列中的第一个任务
        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        //解锁
        m_queuelocker.unlock();
        if ( ! request )
        {
            continue;
        }
        //调用任务处理函数
        request->process();
    }
}

#endif
