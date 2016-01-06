#include <unistd.h>
#include <fcntl.h>

int set_nonblocking(int fd) {
    int old_option=fcntl(fd,F_GETFL);
    int new_option=old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}
