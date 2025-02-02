bool daemonize()
{
    //创建子进程，关闭父进程，这样可以使程序在后台运行
    pid_t pid = fork();
    if ( pid < 0 )
    {
        return false;
    }
    else if ( pid > 0 )
    {
        exit( 0 );
    }
    
    //设置文件权限掩码
    umask( 0 );
    //创建新的会话，设置本进程为进程组的首领
    pid_t sid = setsid();
    if ( sid < 0 )
    {
        return false;
    }
    //切换工作目录
    if ( ( chdir( "/" ) ) < 0 )
    {
        /* Log the failure */
        return false;
    }
    //关闭标准输入、标准输出和标准错误输出设备
    close( STDIN_FILENO );
    close( STDOUT_FILENO );
    close( STDERR_FILENO );
    //将标准输入、标准输出和标准错误输出都定向到/dev/null
    open( "/dev/null", O_RDONLY );
    open( "/dev/null", O_RDWR );
    open( "/dev/null", O_RDWR );
    return true;
}