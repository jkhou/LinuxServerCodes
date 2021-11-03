#include <stdio.h>
void byteorder()
{
	union
	{
		short value;
		char union_bytes[ sizeof( short ) ];
	} test;
	test.value = 0x0102;

	//大端字节序（网络字节序）：整数的高位字节存储在内存的低地址处，低位字节存储在内存的高地址处
	if (  ( test.union_bytes[ 0 ] == 1 ) && ( test.union_bytes[ 1 ] == 2 ) )
	{
		printf( "big endian\n" );
	}
	//小端字节序（主机字节序）：整数的高位字节存储在内存的高地址出，低位字节存储在内存的低地址处
	else if ( ( test.union_bytes[ 0 ] == 2 ) && ( test.union_bytes[ 1 ] == 1 ) )
	{
		printf( "little endian\n" );
	}
	else
	{
		printf( "unknown...\n" );
	}
}