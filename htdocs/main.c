#/*
*主要实现功能，处理浏览器的get请求信息，发送网页文件。处理404、403等错误。
*1.实现绑定本机机器的1024端口作为ReageWeb服务提供网页服务的端口。（避免与机器上装有web服务器产生端口冲突）
*2.实现get获取网页方式。
*3.实现index.html作为网站的首页面
* 作者：Reage
* blog:http://blog.csdn.net/rentiansheng
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>


#define MAX 1024
#define METHOD 20 //获取数据的方式,使用字符串的长度
#define URI 255 //表示URI的最大长度
#define VERSION 15	//HTTP版本好的最大长度
#define TYPE 20     //表示文件的类型的长度


int res_socket;
void app_exit();


/*
@description:开始服务端监听
@parameter
ip:web服务器的地址
port：web服务器的端口
@result：成功返回创建socket套接字标识，错误返回-1

*/
int socket_listen( char *ip, unsigned short int port){
	int res_socket; //返回值
	int res, on;
	struct sockaddr_in address;
	struct in_addr in_ip;
	res = res_socket = socket(AF_INET, SOCK_STREAM, 0);
	setsockopt(res_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET ;
	address.sin_port =htons(port);
	address.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr("127.0.0.1");
	res = bind( res_socket, (struct sockaddr *) &address, sizeof( address ) );
	if(res) { printf( "port is used , not to repeat bind\n" ); exit(101); };
	res = listen(res_socket,5);
	if(res) { printf( "listen port is error ;\n" ); exit( 102 );  };
	return res_socket ;
}

/*
@description:向客户端发送网页头文件的信息
@parameter
conn_socket:套接字描述符。
status:http协议的返回状态码。
@s_status:http协议的状态码的含义
@filetype:向客户端发送的文件类型
*/
void send_http_head(int conn_socket, int status, char *s_status, char *filetype){ 
	char buf[MAX];
	memset(buf, 0, MAX);
	sprintf(buf, "HTTP/1.0 %d %s\r\n", status, s_status);
	sprintf(buf, "%sServer: Reage Web Server\r\n", buf);
	sprintf(buf, "%sContent-Type: %s\r\n\r\n", buf, filetype);
	write(conn_socket, buf, strlen(buf));
} 

/*
@description:向客户端发送错误页面信息
@parameter
	conn_socket:套接字描述符。
	status:http协议的返回状态码。
@s_status:http协议的状态码的含义
@filetype:向客户端发送的文件类型
@msg:错误页面信息内容
*/
void send_page_error(int conn_socket, int status, char *s_status, char *msg){ 

	char buf[MAX] ;
	sprintf(buf, "<html><head></head><body><h1> %s </h1><hr>Reage Web Server 0.01</body></head>", msg);
	send_http_head(conn_socket, status, s_status, "text/html");
	write(conn_socket, buf, strlen(buf));
	
}

/*
*@description:返回要处理的数据格式，要发送的文件格式
*@parameter
*	file：要发送的文件名
*	type_file：用来保存文件格式
*/
void get_file_type (char *file, char * type_file){
	int len = strlen (file);
	char *p = file + len - 1;
	while ( *p != '.')p--;
	p ++;
	len = strlen (p);
	switch (len){
		case 0:
			strcpy (type_file, "text/html");
			return ;
			break;
		case 4:
			if ( !strcasecmp (p, "html")){  
				strcpy (type_file, "text/html");
				return ;
			}
			if( !strcasecmp (p, "jpeg")){
				strcpy (type_file, "image/jpeg");
				return ;
			} 
			break;
		case 3:
			if (!strcasecmp (p, "htm")){
				strcpy (type_file, "text/html");
				return;
			}
			if (!strcasecmp (p, "css")){
				strcpy (type_file, "text/css");
				return ;
			}
			if (!strcasecmp (p,"png")){
					strcpy (type_file, "image/png");
					return ;
			}
			if (!strcasecmp (p, "jpg")){
				strcpy (type_file, "image/jpeg");
				return ;
			}
			if (!strcasecmp (p, "gif")){
				strcpy (type_file, "image/fig");
				return;
			}
			if (!strcasecmp (p, "txt")){
				strcpy (type_file, "text/plain");
				return;
			}
			break;
		case 2:
			if ( !strcasecmp(p, "js")){
				strcpy (type_file, "text/javascript");
				return;
			}
			break;
		default:
			strcpy (type_file, "text/html");
			return ;
			break;

	}
	

}


/*
@description:向客户端发送文件
@parameter
	conn_socket:套接字描述符。
@file:要发送文件路径
*/
int send_html(int conn_socket, char *file){  
	int f;
	char buf[MAX];
	char type_file[TYPE];
	int tmp;
	struct stat file_s;
	//如果file为空，表示发送默认主页。主页暂时固定
	if(0 == strlen(file)){
		strcpy(file, "index.html");
	} 
	//如果获取文件状态失败，表示文件不存的，发送404页面，暂时404页面内容固定。
	if(stat(file, &file_s) ){
		send_page_error(conn_socket, 404, "Not found", "Not found<br/> Reage does not implement this mothod\n");
		return 0;
	}
	//如果不是文件或者无读权限，发送无法读取文件
	if( !(S_ISREG(file_s.st_mode)) || !(S_IRUSR & file_s.st_mode) ){
		send_page_error(conn_socket, 403 , "Forbidden", "Forbidden<br/> Reage couldn't read the file\n");
		return 0;
	}

	get_file_type (file, type_file);
	
	f = open(file, O_RDONLY);
	if(0 > f){ 
		//打开文件失败，发送404页面，其实感觉应该发送5xx，服务器内部错误
		send_page_error(conn_socket, 404, "Not found", "Not found<br/> Reage couldn't read the file\n");
		return 0;
	}
	//发送头文件，现在只提供html页面
	send_http_head(conn_socket, 200, "OK", type_file );
	buf[MAX-1] = 0;//将文件内容缓冲区最后的位设置位结束标志。
	//发送文件的内容
    while( (tmp= read(f, buf, MAX-1)) && EOF != tmp ){
		write(conn_socket, buf, tmp);
 	}
}

/*
@description:提取url中可用的信息。访问的网页和数据访问方式
@parameter：
	conn_socket:与客户端链接的套接字
	uri:要处理的url，注意不是浏览器中的url，而是浏览器发送的http请求
@resutl：
*/
int do_uri_get(int conn_socket, char *uri){
	char *p;
	p=strchr(uri, '?');
	if(p)
	{ *p = 0; p++;}
	//关于get Query_String 为提供处理方法
	else
	send_html(conn_socket, uri);
}

void ulog(char *msg){}

void print(char *msg){
	ulog(msg);
	printf(msg);
}

/*
int start()
{
	char config[] = "reage.conf";
	struct stat file_s;
	int fd ;
	char start;		//表示是否开始读取一个站点的配置，0表示没有开始，递增。结束时清零。
	char node[50];
	char *p;
	if( stat(config, &file_s))
	{print("Reage Web Site not found config file\n");exit(0);}
	if( !S_ISREG(file_s.st_mode) || !(S_IRUSR & file_s.st_mode))
	{ print("Reage conf is error\n"); exit(0); }
	fd = open(config, O_RDONLY);
	while( 0 > read(fd, buf, MAX) )
	{
		p = strchr(buf, '#');
		p = 0;
	}
	return 0;
}*/



int main(int argc, char * argv[] ){  
	int  conn_socket;
	int tmp ;
	int line ;
	struct sockaddr_in client_addr;
	char buf[MAX];
	char buf_u[MAX];
	int len = sizeof(client_addr);
	char method[METHOD], uri[MAX], version[VERSION], type[TYPE];
	char pwd[1024];
	char *p;
	res_socket = socket_listen( "127.0.0.1", 1024) ;
	//当按ctrl+c结束程序时调用，使用app_exit函数处理退出过程
	signal(SIGINT, app_exit);
	while(1){
		conn_socket = accept( res_socket, (struct sockaddr * )&client_addr, &len );
		printf("reage\n");
		line = 0;
		tmp = read (conn_socket, buf, MAX-1);
		buf [MAX - 1] = 0;
		printf ("%s\n", buf);
		//send_http_head(conn_socket, 200, "text/html");
		sscanf(buf, "%s %s %s", method, uri, version);
		//目前只处理get请求
		if(!strcasecmp(method, "get"))
		//send_html(conn_socket, "h.html");
		do_uri_get(conn_socket, uri+1);
		close(conn_socket);
  	}

}

void app_exit(){
	//回复ctrl+c组合键的默认行为
	signal (SIGINT, SIG_DFL);
	//关闭服务端链接、释放服务端ip和端口
	close(res_socket);
	printf("\n");
	exit(0);
}


