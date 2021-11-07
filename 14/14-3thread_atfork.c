#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <wait.h>

pthread_mutex_t mutex;
//子线程运行的函数。先获得互斥锁，5s之后再释放互斥锁
void* another( void* arg )
{
    printf( "in child thread, lock the mutex\n" );
    pthread_mutex_lock( &mutex );
    sleep( 5 );
    pthread_mutex_unlock( &mutex );
}

void prepare()
{
    pthread_mutex_lock( &mutex );
}

void infork()
{
    pthread_mutex_unlock( &mutex );
}

int main()
{
    pthread_mutex_init( &mutex, NULL );
    pthread_t id;
    // pthread_create( &id, NULL, another, NULL );
    pthread_atfork( prepare, infork, infork );
    //父进程中的主线程暂停1s，以确保在执行fork操作之前，子线程已经开始运行并获得了互斥锁mutex
    sleep( 1 );
    //创建子进程
    int pid = fork();
    if( pid < 0 )
    {
        pthread_join( id, NULL );
        pthread_mutex_destroy( &mutex );
        return 1;
    }
    //子进程从父进程中继承了mutex处于上锁的状态，这是由父进程中的子线程pthread_mutex_lock引起的
    else if( pid == 0 )
    {
        printf( "I anm in the child, want to get the lock\n" );
        //加锁操作会一直阻塞
        pthread_mutex_lock( &mutex );
        printf( "I can not run to here, oop...\n" );
        pthread_mutex_unlock( &mutex );
        exit( 0 );
    }
    else
    {
        pthread_mutex_unlock( &mutex );
        wait( NULL );
    }
    pthread_join( id, NULL );
    pthread_mutex_destroy( &mutex );
    return 0;
}
