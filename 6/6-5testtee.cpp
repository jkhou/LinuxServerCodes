#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

int main( int argc, char* argv[] )
{
	if ( argc != 2 )
	{
		printf( "usage: %s <file>\n", argv[0] );
		return 1;
	}
	int filefd = open( argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0666 );
	assert( filefd > 0 );

	int pipefd_stdout[2];
	//创建终端管道
    int ret = pipe( pipefd_stdout );
	assert( ret != -1 );

	int pipefd_file[2];
	//创建文件管道
    ret = pipe( pipefd_file );
	assert( ret != -1 );

	while(1){
		//将标准输入内容输入管道pipefd_stdout
		ret = splice( STDIN_FILENO, NULL, pipefd_stdout[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE );
		assert( ret != -1 );
		//tee函数在两个文件描述符之间复制数据，也是零拷贝
		//tee将管道pipefd_stdout的输出复制到pipefd_file
		ret = tee( pipefd_stdout[0], pipefd_file[1], 32768, SPLICE_F_NONBLOCK ); 
		assert( ret != -1 );
		//将管道pipefd_file的输出定向到文件描述符filefd，从而将标准输入的内容写入文件
		ret = splice( pipefd_file[0], NULL, filefd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE );
		assert( ret != -1 );
		//将管道pipefd_stdout的输出定向到标准输出，其内容和写入文件的内容保持一致
		ret = splice( pipefd_stdout[0], NULL, STDOUT_FILENO, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE );
		assert( ret != -1 );
	}

	close( filefd );
    close( pipefd_stdout[0] );
    close( pipefd_stdout[1] );
    close( pipefd_file[0] );
    close( pipefd_file[1] );
	return 0;
}
