#include "ws/core/Utils.h"

#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#endif

namespace ws::core
{
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
	bool createPidfile(const char* pidfile)
	{
		if (!pidfile) {
			return false;
		}
		int fd = open(pidfile, O_RDWR | O_CREAT, 0664);
		if (fd < 0)
			return false;

		flock lock;
		lock.l_type = F_WRLCK;
		lock.l_whence = SEEK_SET;
		lock.l_start = 0;
		lock.l_len = 0;
        lock.l_pid = getpid();
		if (fcntl(fd, F_SETLK, &lock))
		{
			close(fd);
			return false;
		}
		char buf[32] = { 0 };
		sprintf(buf, "%d\n", lock.l_pid);
		size_t pidlength = strlen(buf);
		if (write(fd, buf, pidlength) != pidlength)
		{
			close(fd);
			return false;
		}
		//关闭fd会导致写锁失效
		return true;
	}
#endif

}