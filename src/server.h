#include "thread_pool.h"

#define MAX_ADDR_LEN 16
#define MAX_PATH_SIZE 256

struct config {
	unsigned int port;
	unsigned int thread_num;
	char *addr;
	char *htdocs;
	char *config_path;
};

struct server {
	struct config conf;
	int srv_fd;
	int epoll_fd;
	thpool_t *tp_p;
}
