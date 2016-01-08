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

#include "http.h"

static struct {
        char *ext;
        char *type;
} extensions [] = { 
        {"gif", "image/gif" },  
        {"jpg", "image/jpg" },  
        {"jpeg","image/jpeg"},
        {"png", "image/png" },  
        {"ico", "image/ico" },  
        {"zip", "image/zip" },  
        {"gz",  "image/gz"  },  
        {"tar", "image/tar" },  
        {"htm", "text/html" },  
        {"html","text/html" }, 
		{"css","text/css"},
		{"js","text/javascript"},
        {0,0} };

void http_request_parse(char *rb_buf,struct http_request *req) {
    int pos=0,i;

    i=0;
    while(isalpha(rb_buf[pos]) && i<(MAX_METHOD_SIZE-1)) {
        req->method[i++]=rb_buf[pos++];
    }
    req->method[i]='\0';

    while(isspace(rb_buf[pos])) pos++;

    i=0;
    while(!isspace(rb_buf[pos]) && i<(MAX_URI_SIZE-1)) {
        req->uri[i++]=rb_buf[pos++];
    }
    req->uri[i]='\0';

    if(strcmp("POST",req->method) == 0) {
        char *p=strstr(rb_buf,"Content-Length: ");
        i=0;p+=strlen("Content-Length: ");
        while(isdigit(*p)) {
            req->content_length[i]=*p;
            i++,p++;
        }
        req->content_length[i]='\0';

        while(*p=='\r' || *p=='\n') p++;
        strncpy(req->content,p,MAX_CONTENT_SIZE);
    }
}

static void get_filetype(char *uri,char **filetype)
{
    int i,len,uri_len;

    uri_len=strlen(uri);
    
    for(i=0;extensions[i].ext != 0;i++) {
        len=strlen(extensions[i].ext);
        if(!strncmp(&uri[uri_len-len],extensions[i].ext,len)) {
            *filetype=extensions[i].type;
            break;
        }
    }   
}

static void send_response_hdr(int cli_fd,int status,char *s_status,char *filetype)
{
    char header_buff[MAX_HEADER_SIZE];
    memset(header_buff,0,MAX_HEADER_SIZE);
    sprintf(header_buff, "HTTP/1.0 %d %s\r\n", status, s_status);  
    sprintf(header_buff, "%sServer: Coix Web Server\r\n", header_buff);
    sprintf(header_buff, "%sContent-Type: %s\r\n\r\n", header_buff, filetype);
    write(cli_fd, header_buff, strlen(header_buff));  
}

static void send_response_content(int cli_fd, int fd) {
    char response_content[MAX_BUF_SIZE];
    int n,pos=0;
    do {
        n=read(fd,response_content+pos,MAX_BUF_SIZE-pos);
        pos+=n;
    }while(n > 0);
    response_content[pos]='\0'; //just for safety
    write(cli_fd,response_content,pos);
}

static void srv_file(int cli_fd, char *file_path) {
    char *filetype;
    get_filetype(file_path,&filetype);
    send_response_hdr(cli_fd,200,"OK",filetype);
    int fd=open(file_path,O_RDONLY);
    send_response_content(cli_fd,fd);
    
}

static void cgi_get(int cli_fd, char *file_path, char *query_string) {
    log_info("cgi_get() file_path:%s query_string:%s\n",file_path,query_string);
    int in_fd[2],out_fd[2];
    if(pipe(in_fd)<0 || pipe(out_fd)<0) {
        log_error("pipe() error\n");
    }
    pid_t pid=fork();
    if(pid == 0) {
        close(in_fd[1]);
        close(out_fd[0]);
        setenv("QUERY_STRING",query_string,1);
        dup2(in_fd[0],STDIN_FILENO);
        dup2(out_fd[1],STDOUT_FILENO);
        close(in_fd[0]);
        close(out_fd[1]);
        execl(file_path,file_path,NULL);
    } else if(pid > 0) {
        close(in_fd[0]);
        close(out_fd[1]);
        char response_content[MAX_CONTENT_SIZE];
        int n,pos=0;
        while((n=read(out_fd[0],response_content+pos,MAX_CONTENT_SIZE-pos)) > 0) {
            //printf("n=%d\n",n);
            pos+=n;
        }
        //printf("n=%d\n",n);
        //response_content[pos]='\0';
        log_info("cgi_get() response_content:%s",response_content);
        send_response_hdr(cli_fd,200,"OK","text/html");
        write(cli_fd,response_content,pos);
    }
}

static void cgi_post(int cli_fd, char *file_path, struct http_request *req) {
    log_info("file_path:%s\n",file_path);
    int in_fd[2],out_fd[2];
    if(pipe(in_fd)<0 || pipe(out_fd)<0) {
        log_error("pipe() error\n");
        return;
    }
    pid_t pid=fork();
    if(pid == 0) {
        log_info("child file_path:%s\n",file_path);
        close(in_fd[1]);
        close(out_fd[0]);
        setenv("CONTENT_LENGTH",req->content_length,1);
        if(dup2(in_fd[0],STDIN_FILENO) != STDIN_FILENO) {
            perror("dup2 stdin error");
            log_error("cgi_post() dup2 stdin error");
        }
        close(in_fd[0]);
        
        if(dup2(out_fd[1],STDOUT_FILENO) != STDOUT_FILENO) {
            perror("dup2 stdout error");
            log_error("cgi_post() dup2 stdout error");
        }
        close(out_fd[1]);
        
        
        execl(file_path,file_path,NULL);

    } else if(pid > 0) {
        close(in_fd[0]);
        close(out_fd[1]);
        log_info("content_length:%s content:%s\n",req->content_length,req->content);
        
        if(write(in_fd[1],req->content,atoi(req->content_length))<0) {
            perror("write() error\n");
        }
        char response_content[MAX_CONTENT_SIZE];
        int n,pos=0;
        //perror("read() error");
        while((n=read(out_fd[0],response_content+pos,MAX_CONTENT_SIZE-pos)) > 0) {
            //printf("n=%d\n",n);
            pos+=n;
        }
        //printf("n=%d\n",n);
        //perror("read() error");
        //response_content[pos]='\0';
        log_info("cgi_post() response_content:%s\n",response_content);
        send_response_hdr(cli_fd,200,"OK","text/html");
        write(cli_fd,response_content,pos);
    }
}

void send_response(int cli_fd, struct http_request *req, struct config *conf_p) {
    char *htdocs=conf_p->htdocs;
    if(strncmp("GET",req->method,strlen("GET")) == 0) {
        char *query_string=req->uri;
        int cgi_flag=0;
        while((*query_string)!='?' && (*query_string)!='\0') {
            //printf("in while,%c\n",*query_string);
            query_string++;
        }
        //printf("here\n");     
        char file_path[MAX_PATH_SIZE];
        if(*query_string == '?') {
            *query_string='\0';
            query_string++;
            cgi_flag=1;
        }
        snprintf(file_path,MAX_PATH_SIZE,"%s%s",htdocs,req->uri);
        if(file_path[strlen(file_path)-1] == '/')
            strcat(file_path,"index.html");
        log_info("file_path:%s\n",file_path);
        struct stat st;
        if(stat(file_path,&st) == -1) {
            printf("no file\n");
        } else {
            printf("file_path:%s\n",file_path);
            if(cgi_flag == 0)
                srv_file(cli_fd,file_path);
            else
                cgi_get(cli_fd,file_path,query_string);
        }

    } else if(strcmp("POST",req->method) == 0) {
        char file_path[MAX_PATH_SIZE];
        snprintf(file_path,MAX_PATH_SIZE,"%s%s",htdocs,req->uri);
        struct stat st;
        log_info("filepath:%s\n",file_path);
        if(stat(file_path,&st) == -1) {
            perror("stat() error");
        } else {
            cgi_post(cli_fd,file_path,req);
        }
        
    } 
}
