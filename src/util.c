#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

int set_nonblocking(int fd) {
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

int epoll_add(int epoll_fd, int srv_fd) {
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd=srv_fd;
	return epoll_ctl(epoll_fd,EPOLL_CTL_ADD,srv_fd,&ev);
}
