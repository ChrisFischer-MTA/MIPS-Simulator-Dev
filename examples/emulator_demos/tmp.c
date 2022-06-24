#include <sys/syscall.h>
#include <unistd.h>
int main(void)
{
	SYS_set_thread_area(10);
	return 0;
}
