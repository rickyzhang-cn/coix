#define MAX_ADDR_LEN 16
#define MAX_PATH_SIZE 256

struct config {
	unsigned int port;
	unsigned int thread_num;
	char *addr;
	char *htdocs;
	char *log_path;
	char *config_path;
};
