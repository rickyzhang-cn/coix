#include "logger.h"
#include "server.h"

int listenfd_create(struct config *conf_p) {
	int srv_fd=socket(AF_INET,SOCK_STREAM,0);
	if(srv_fd == -1)
		log_error("socekt() fails");

	struct sockaddr_in srv_addr;
	memset(&srv_addr,0,sizeof(srv_addr));
	srv_addr.sin_family=AF_INET;
	srv_addr.sin_port=htons(conf_p->port);
	if(conf_p->addr != NULL)
		srv_addr.sin_addr.s_addr=inet_addr(conf_p->addr);
	else
		srv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	if(bind(srv_fd,&srv_addr,sizeof(srv_addr)) == -1)
		log_error("bind() fails");

	if(listen(srv_fd,5) == -1) {
		log_error("listen() fails");
	}

	return srv_fd;
}

void cmd_options_parse(int argc, char *argv[]) {
	int c;
	while(1) {
		static struct option long_options[] = {
			{.name="port", .has_arg=1, .flag=NULL, .val='p'},
			{.name="addr", .has_arg=1, .flag=NULL, .val='a'},
			{.name="config", .has_arg=1, .flag=NULL, .val='c'},
			{.name="num", .has_arg=1, .flag=NULL, .val='n'},
			{.name="htdocs", .has_arg=1, .flag=NULL, .val='d'},
			{.name=NULL, .has_arg=0, .flag=NULL, .val='\0'}
		};
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
				conf->config_path=strdup(optarg);
				break;
			case 'n':
				conf->thread_num=strtoul(optarg,NULL,0);
				break;
			case 'd':
				conf->htdocs=strdup(optarg);
				break;
		}
	}
}

void process_value(json_value *value, struct config *conf_p) {
	if(value->type != json_object) {
		log_warn("process_value() value is not json_object");
		return;
	}

	int len=value->u.object.length;
	for(int i=0;i<len;i++) {
		char *key=value->u.object.value[i].name;
		if(!strcmp(key,"port")) {
			conf_p->port=value->u.object.value[i].value->u.integer;
		} else if(!strcmp(key,"addr")) {
			conf_p->addr=strdup(value->u.object.value[i].value->u.string.ptr);
		} else if(!strcmp(key,"thread_num")) {
			conf_p->thread_num=value->u.object.value[i].value->u.integer;
		} else if(!strcmp(key,"htdocs")) {
			conf_p->htdocs=strdup(value->u.object.value[i].value->u.string.ptr);
		} else {
			log_warn("has not this option:%s",key);
		}
	}
}

void json_options_parse(struct config *conf_p) {
	struct stat st;
	if(stat(conf_p->config_path,&st) == -1) {
		log_warn("config file %s not exist",conf_p->config_path);
		return;
	}
	int file_size=st.st_size;
	char *file_content=(char *)malloc(file_size);
	if(file_content == NULL) {
		log_error("json_options_parse() malloc fails");
	}
	FILE *fp=fopen(filename,"rt");
	if(fp == NULL) {
		free(file_content);
		log_warn("json_options_parse() fopen fails");
		return;
	}
	if(fread(file_content,file_size,1,fp) != 1) {
		free(file_content);
		fclose(fp);
		log_warn("json_options_parse fread fails");
		return;
	}
	fclose(fp);

	json_char *json=(json_char *)file_content;
	json_value *value=json_parse(json,file_size);

	if(json_value == NULL) {
		free(file_content);
		log_warn("json_options_parse() json_parse fails");
		return;
	}

	process_value(value,conf_p);

	json_value_free(value);
	free(file_content);
}

int epoll_add(int epoll_fd, int srv_fd) {
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd=srv_fd;
	return epoll_ctl(epoll_fd,EPOLL_CTL_ADD,srv_fd,&ev);
}

void coix_init(int argc, char *agrv[], struct server *srv_p) {
	if(logger_create() != true) {
		printf("logger_create() fails\n");
		exit(EXIT_FAILURE);
	}
	
	cmd_options_parse(argc,argv,&srv_p->config);

	if(srv_p->conf.config_path != NULL)
		json_options_parse(&srv_p->config);

	srv_p->srv_fd=listenfd_create(&srv_p->config);

	set_nonblocking(srv_p->srv_fd);

	srv_p->tp_p=thpool_init(srv_p->config.thread_num);
	if(srv_p->tp_p == NULL)
		log_error("thpool_init() fails");
	
	srv_p->epoll_fd=epoll_create1(0);
	if(srv_p->epoll_fd == -1)
		log_error("epoll_create1() fails");

	if(epoll_add(srv_p->epoll_fd,srv_p->srv_fd) == -1)
		log_error("epoll_add() fails");
}

int main(int argc, char *argv[]) {	
	struct server *srv_p=(struct server *)malloc(sizeof(struct server));
	if(srv_p == NULL) {
		printf("srv_p create fails\n");
		exit(EXIT_FAILURE);
	}
	memset(srv_p,0,sizeof(struct server));

	coix_init(argc,argv,srv_p);

	int epoll_fd=srv_p->epoll_fd;
	thpool_t *tp_p=srv_p->tp_p;
	while(1) {
		struct epoll_event events[MAX_EVENT_NUM];
		int n=epoll_wait(epoll_fd,events,MAX_EVENT_NUM,-1);
		for(int i=0;i<n;i++) {
			if(events[i].data.fd == srv_fd) {
				struct sockaddr_in cli_addr;
				int len=sizeof(cli_addr);
				int cli_fd=accept(srv_fd,&cli_addr,&len);
				if(cli_fd == -1) {
					log_warn("accept() cli_fd=-1");
					continue;
				}
				thpool_add_work(tp_p,cli_handler,&cli_fd);
			} else {
				log_warn("should not happen:srv_fd=%d,events[%d].data.fd=%d"
						,srv_fd,i,events[i].data.fd);
			}	
		}
	}
}
