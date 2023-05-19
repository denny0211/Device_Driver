#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <errno.h> 

int main(int argc, char* argv[])
{
	int ret = mknod("/dev/testdev1",S_IFCHR | S_IRUSR | S_IWUSR | S_IROTH,(240<<8)|1);

	if(ret<0)
	{
		perror("mknod()");
		return 1;
	}
	ret = open("/dev/testdev1",O_RDWR);
	if(ret<0)
	{
		perror("open()");
		//return 1;
		return EPERM;
	}
	return 0;
}
