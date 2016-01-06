#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/stat.h>

#include "json.h"
#include "options_parser.h"
#include "logger.h"

void cmd_options_parse(int argc, char *argv[], struct config *conf_p) {
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
				conf_p->port=strtoul(optarg,NULL,0);
				break;
			case 'a':
				conf_p->addr=strdup(optarg);
				break;
			case 'c':
				conf_p->config_path=strdup(optarg);
				break;
			case 'n':
				conf_p->thread_num=strtoul(optarg,NULL,0);
				break;
			case 'd':
				conf_p->htdocs=strdup(optarg);
				break;
		}
	}
}

static void process_value(json_value *value, struct config *conf_p) {
	if(value->type != json_object) {
		log_warn("process_value() value is not json_object");
		return;
	}

	int len=value->u.object.length;
	for(int i=0;i<len;i++) {
		char *key=value->u.object.values[i].name;
		if(!strcmp(key,"port")) {
			conf_p->port=value->u.object.values[i].value->u.integer;
		} else if(!strcmp(key,"addr")) {
			conf_p->addr=strdup(value->u.object.values[i].value->u.string.ptr);
		} else if(!strcmp(key,"thread_num")) {
			conf_p->thread_num=value->u.object.values[i].value->u.integer;
		} else if(!strcmp(key,"htdocs")) {
			conf_p->htdocs=strdup(value->u.object.values[i].value->u.string.ptr);
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
	FILE *fp=fopen(conf_p->config_path,"rt");
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

	if(value == NULL) {
		free(file_content);
		log_warn("json_options_parse() json_parse fails");
		return;
	}

	process_value(value,conf_p);

	json_value_free(value);
	free(file_content);
}
