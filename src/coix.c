#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <ctype.h>

#include "logger.h"
#include "server.h"
#include "http.h"
#include "util.h"
#include "options_parser.h"
#include "thread_pool.h"

#define MAX_EVENT_NUM 1024

static struct server *g_srv_p;
char *default_config_file="/home/rickyzhang/coix/conf/coix.json";

int listenfd_create(struct config *conf_p) {
	int srv_fd=socket(AF_INET,SOCK_STREAM,0);
	if(srv_fd == -1)
		log_error("socekt() fails");

    int reuse = 1;
    setsockopt(srv_fd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
	
    struct sockaddr_in srv_addr;
	memset(&srv_addr,0,sizeof(srv_addr));
	srv_addr.sin_family=AF_INET;
	srv_addr.sin_port=htons(conf_p->port);
	if(conf_p->addr != NULL)
		srv_addr.sin_addr.s_addr=inet_addr(conf_p->addr);
	else
		srv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(srv_fd,(struct sockaddr *)&srv_addr,sizeof(srv_addr)) == -1)
		log_error("bind() fails");

	if(listen(srv_fd,5) == -1) {
		log_error("listen() fails");
	}

	return srv_fd;
}


int epoll_add(int epoll_fd, int srv_fd) {
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd=srv_fd;
	return epoll_ctl(epoll_fd,EPOLL_CTL_ADD,srv_fd,&ev);
}

void coix_init(int argc, char *argv[], struct server *srv_p) {
	if(logger_create() != true) {
		printf("logger_create() fails\n");
		exit(EXIT_FAILURE);
	}
	
	cmd_options_parse(argc,argv,&srv_p->conf);

	if(srv_p->conf.config_path == NULL)
        srv_p->conf.config_path=default_config_file;
	json_options_parse(&srv_p->conf);

	srv_p->srv_fd=listenfd_create(&srv_p->conf);

	set_nonblocking(srv_p->srv_fd);

	srv_p->tp_p=thpool_init(srv_p->conf.thread_num);
	if(srv_p->tp_p == NULL)
		log_error("thpool_init() fails");
	
	srv_p->epoll_fd=epoll_create1(0);
	if(srv_p->epoll_fd == -1)
		log_error("epoll_create1() fails");

	if(epoll_add(srv_p->epoll_fd,srv_p->srv_fd) == -1)
		log_error("epoll_add() fails");
}

void handle_request(int cli_fd, char *rd_buf) {
    printf("%s",rd_buf);
    struct http_request req;
    http_request_parse(rd_buf,&req);
    printf("method:%s uri:%s\n",req.method,req.uri);
    send_response(cli_fd,&req,&g_srv_p->conf);
    close(cli_fd);
}

void* cli_handler(void *arg) {
    int cli_fd=*((int *)arg);
    char rd_buf[MAX_BUF_SIZE];
    int n=read(cli_fd,rd_buf,MAX_BUF_SIZE);
    handle_request(cli_fd,rd_buf);
}

int main(int argc, char *argv[]) {	
	struct server *srv_p=(struct server *)malloc(sizeof(struct server));
	if(srv_p == NULL) {
		printf("srv_p create fails\n");
		exit(EXIT_FAILURE);
	}
	memset(srv_p,0,sizeof(struct server));

	coix_init(argc,argv,srv_p);

    g_srv_p=srv_p;

	int epoll_fd=srv_p->epoll_fd;
    int srv_fd=srv_p->srv_fd;
	thpool_t *tp_p=srv_p->tp_p;
	while(1) {
		struct epoll_event events[MAX_EVENT_NUM];
		int n=epoll_wait(epoll_fd,events,MAX_EVENT_NUM,-1);
        log_info("epoll start");
		for(int i=0;i<n;i++) {
            log_info("epoll n=%d",n);
			if(events[i].data.fd == srv_fd) {
				struct sockaddr_in cli_addr;
				int len=sizeof(cli_addr);
                while(1) {
				    int cli_fd=accept(srv_fd,(struct sockaddr *)&cli_addr,&len);
                    log_info("cli_fd=%d",cli_fd);
				    if(cli_fd == -1) {
					    if(errno==EAGAIN || errno==EWOULDBLOCK) {
                            log_info("epoll break");
					        break;
                        }
                        if(errno == EINTR)
                            continue;
				    }
				    thpool_add_work(tp_p,cli_handler,&cli_fd);
                }
			} else {
				log_warn("should not happen:srv_fd=%d,events[%d].data.fd=%d"
						,srv_fd,i,events[i].data.fd);
			}	
		}
	}
}
