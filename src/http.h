#ifndef __HTTP_H_
#define __HTTP_H_

#define MAX_BUF_SIZE (64*1024)
#define MAX_METHOD_SIZE 5
#define MAX_URI_SIZE 2048
#define MAX_PATH_SIZE 256
#define MAX_HEADER_SIZE 2048
#define MAX_CONTENT_SIZE 2048
#define MAX_CL_SIZE 32

struct http_request {
	char method[MAX_METHOD_SIZE];
	char uri[MAX_URI_SIZE];
	char content_length[MAX_CL_SIZE];
	char content[MAX_CONTENT_SIZE];
};


void http_request_parse(char *rb_buf,struct http_request *req);
void send_response(int cli_fd, struct http_request *req);

#endif
