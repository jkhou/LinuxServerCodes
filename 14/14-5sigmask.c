    #include <pthread.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
    #include <signal.h>
    #include <errno.h>

    /* Simple error handling functions */

    #define handle_error_en(en, msg) \
            do { errno = en; perror(msg); exit(EXIT_FAILURE); } while (0)

    static void *
    sig_thread(void *arg)
    {
        printf( "yyyyy, thread id is: %ld\n", pthread_self() );
        sigset_t aset;
        int s, sig;
        sigemptyset(&aset);
        sigaddset(&aset, SIGQUIT);
        sigaddset(&aset, SIGUSR1);
        //s = pthread_sigmask(SIG_BLOCK, &aset, NULL);
        sigset_t *set = (sigset_t *) arg;

        //第二部，调用sigwait等待函数
        for (;;) {
            s = sigwait(set, &sig);
            if (s != 0)
                handle_error_en(s, "sigwait");
            printf("Signal handling thread got signal %d\n", sig);
        }
    }

static void handler( int arg )
{
    printf( "xxxxx, thread id is: %ld\n", pthread_self() );
}

    int main(int argc, char *argv[])
    {
        pthread_t thread;
        sigset_t set;
        int s;

        /* Block SIGINT; other threads created by main() will inherit
*               a copy of the signal mask. */

        signal( SIGQUIT, handler );
//           if (s != 0)
//             handle_error_en(s, "pthread_sigmask");

        //第一步，在主线程中设置信号掩码
        sigemptyset(&set);
        sigaddset(&set, SIGQUIT);
        sigaddset(&set, SIGUSR1);
        s = pthread_sigmask(SIG_BLOCK, &set, NULL);
        if (s != 0)
            handle_error_en(s, "pthread_sigmask");
        printf( "sub thread with id: %ld\n", thread );

        s = pthread_create(&thread, NULL, &sig_thread, (void *) &set);
        if (s != 0)
            handle_error_en(s, "pthread_create");
        printf( "sub thread with id: %ld\n", thread );
        /* Main thread carries on to create other threads and/or do
*               other work */

        pause();            /* Dummy pause so we can test program */
    }

