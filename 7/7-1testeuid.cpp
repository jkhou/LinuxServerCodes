#include <unistd.h>
#include <stdio.h>

int main()
{
    //获取真实用户ID
    uid_t uid = getuid();
    //获取有效用户ID
    uid_t euid = geteuid();
    printf( "userid is %d, effective userid is: %d\n", uid, euid );
    return 0;
}
