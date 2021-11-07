#include <sys/sem.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

union semun
{
     int val;                  
     struct semid_ds* buf;     
     unsigned short int* array;
     struct seminfo* __buf;    
};

//op为-1时执行P操作，op为1时执行V操作
void pv( int sem_id, int op )
{
    struct sembuf sem_b;
    sem_b.sem_num = 0;
    sem_b.sem_op = op;
    sem_b.sem_flg = SEM_UNDO;
    semop( sem_id, &sem_b, 1 );
}

int main( int argc, char* argv[] )
{
    //创建一个新的信号量集，其中信号量数目为1
    int sem_id = semget( IPC_PRIVATE, 1, 0666 );

    union semun sem_un;
    sem_un.val = 1;
    //将信号量集中第0个信号量的semval设置为sem_un.val
    semctl( sem_id, 0, SETVAL, sem_un );

    //在父子进程之间共享IPC_PRIVATE的关键就在于二者都可以操作该信号量的标识符sem_id
    pid_t id = fork();
    //创建子进程失败
    if( id < 0 )
    {
        return 1;
    }
    //在子进程中返回值是0
    else if( id == 0 )
    {
        printf( "child try to get binary sem\n" );
        pv( sem_id, -1 );
        printf( "child get the sem and would release it after 5 seconds\n" );
        sleep( 5 );
        pv( sem_id, 1 );
        exit( 0 );
    }
    //在父进程中返回值是子进程PID
    else
    {
        printf( "parent try to get binary sem\n" );
        pv( sem_id, -1 );
        printf( "parent get the sem and would release it after 5 seconds\n" );
        sleep( 5 );
        pv( sem_id, 1 );
    }

    waitpid( id, NULL, 0 );
    //立即删除信号量集，并唤醒所有等待该信号量集的进程
    semctl( sem_id, 0, IPC_RMID, sem_un );
    return 0;
}
