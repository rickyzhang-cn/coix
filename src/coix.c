#include "logger.h"

int listenfd_create(struct config *conf_p) {
	int srv_fd=socket(AF_INET,SOCK_STREAM,0);
	if(srv_fd < 0) {
		log_error();
	}

	struct sockaddr_in srv_addr;
	memset(&srv_addr,0,sizeof(srv_addr));
	srv_addr.sin_family=AF_INET;
	srv_addr.sin_port=htons(conf_p->port);
	if(conf_p->addr != NULL)
		srv_addr.sin_addr.s_addr=inet_addr(conf_p->addr);
	else
		srv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(srv_fd,&srv_addr,sizeof(srv_addr)) < 0) {
		log_error();
	}

	if(listen(srv_fd,5) < 0) {
		log_error();
	}

	return srv_fd;
}

void cmd_options_parse(int argc, char *argv[]) {
	int c;
	struct option long_options[] = {
		{.name="port", .has_arg=1, .val='p'},
		{.name="addr", .has_arg=1, .val='a'},
		{.name="config", .has_arg=1, .val='c'},
		{.name="num", .has_arg=1, .val='n'},
		{.name="htdocs", .has_arg, .val='d'},
		{.name="log", .has_arg=1, .val='l'}
	};
	while(1) {
		c=getopt_long(argc,argv,"p:a:c:n:d:",long_options,NULL);
		if(c == -1)
			break;
		switch(c) {
			case 'p':
				conf->port=strtoul(optarg,NULL,0);
				break;
			case 'a':
				conf->addr=strdup(optarg);
				break;
			case 'c':
				conf->c
		}
	}
}

void json_options_parse(struct config *conf_p) {
	struct stat st;
	if(stat(conf_p->config_path,&st) == -1) {
		log_error();
	}
	int file_size=st.st_size;
	char *file_content=(char *)malloc(file_size);
	if(file_content == NULL) {
		log_error();
	}
	FILE *fp=fopen(filename,"rt");
	if(fp == NULL) {
		log_error();
		free(file_content);
	}
	if(fread(file_content,file_size,1,fp) != 1) {
		log_error();
		free(file_content);
		fclose(fp);
	}
	fclose(fp);

	json_char *json=(json_char *)file_content;
	json_value *value=json_parse(json,file_size);

	if(json_value == NULL) {
		log_error();
		free(file_content);
	}

	process_value(value,0);

	json_value_free(value);
	free(file_content);
}

int main(int argc, char *argv[]) {	
	if(logger_create() == -1) {
		printf("logger_create() fails\n");
		return EXIT_FAILURE;
	}

	struct config *conf=(struct config *)malloc(sizeof(struct config));
	if(conf == NULL) {
		log_error();
	}
	memset(conf,0,sizeof(conf));
	cmd_options_parse(argc,argv,conf);
	if(conf->config_path != NULL)
		json_options_parse(conf);

	int srv_fd=listenfd_create(conf_p);
	if(srv_fd < 0) {
		log_error();
	}
	set_nonblocking(srv_fd);

	thpool_t *tp_p=thpool_init(64);
	if(tp_p == NULL) {
	}
	
	int epoll_fd=epoll_create1(0);
	struct epoll_event ev;
	ev.events=EPOLLIN | EPOLLET;
	ev.data.fd=srv_fd;
	epoll_ctl(epoll_fd,EPOLL_CTL_ADD,srv_fd,&ev);

	while(1) {
		struct epoll_event events[1024];
		int n=epoll_wait(epoll_fd,events,1024,-1);
		for(int i=0;i<n;i++) {
			if(events[i].data.fd == srv_fd) {
				struct sockaddr_in cli_addr;
				int len=sizeof(cli_addr);
				int cli_fd=accept(srv_fd,&cli_addr,&len);
				if(cli_fd < 0) {
					log_error();
				}
				thpool_add_work(tp_p,cli_handler,&cli_fd);
			} else {
				log_error();
			}	
		}
	}
}
