
#include<sys/socket.h>
#include<iostream>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<netinet/in.h>
#include<cstring>
#include<string>
#include<algorithm>
#include<fcntl.h>
#include<pthread.h>


constexpr in_port_t used_port = 8000;
#define SERVER_PATH "/home/rankzero/Desktop"

void _out(const char* mesg)
{
	std::cout << mesg << std::endl;
}


int headers(int connfd , const char* filename , int size)
{
	char buf [1024];
	strcpy(buf , "HTTP/1.0 200 OK\r\n");
	send(connfd , buf , strlen(buf) , 0);
	strcpy(buf , "Server:Grt\r\n");
	send(connfd , buf , strlen(buf) , 0);
	strcpy(buf , "Content-Type: text/html\r\n");
	send(connfd , buf , strlen(buf) , 0);
	sprintf(buf , "Content-length:%d\r\n" , size);
	send(connfd , buf , strlen(buf) , 0);
	strcpy(buf , "\r\n");
	send(connfd , buf , strlen(buf) , 0);
	return 0;
}

void request_fail(int connfd)
{
	char buf [1024];
	strcpy(buf , "HTTP/1.0 404 NOT FOUND\r\n");
	send(connfd , buf , strlen(buf) , 0);
	strcpy(buf , "Content-Type: text/html\r\n");
	send(connfd , buf , strlen(buf) , 0);
	sprintf(buf , "Content-length:%d\r\n",62);
	send(connfd , buf , strlen(buf) , 0);
	strcpy(buf , "\r\n");
	send(connfd , buf , strlen(buf) , 0);
	strcpy(buf , "<html><tile>Not Found</title>\r\n");
	send(connfd , buf , strlen(buf) , 0);
	strcpy(buf , "<body>404 not found</body></html>\r\n");
	send(connfd , buf , strlen(buf) , 0);
}

void handle_request(void* arg)
{
	int connfd = (intptr_t)arg;

	char buf [1024];
	char method [100];
	char filename [100];
	char args [100];
	char* start;
	char* end;
	bool is_static = true;

	int rec_ret = recv(connfd , buf , 100 , 0);
	end = std::find(buf , &buf [100] , ' ');
	*(std::copy(buf , end , method)) = '\0'; //method
	start = end;
	++start;

	_out("recv");

	filename [0] = '\0';
	strcat(filename , SERVER_PATH);
	end = std::find(start , &buf [100] , '?');
	if (end != (&buf [100]))
	{
		*(std::copy(start , end , filename + strlen(filename))) = '\0';
		is_static = false;
	}
	else //can't find '?'
	{
		end = std::find(start , &buf [100] , ' ');
		*(std::copy(start , end , filename + strlen(filename))) = '\0';
	}
	if (*(end - 1) == '/')
	{
		strcat(filename , "index.html");
	}



	int filefd = open(filename , O_RDONLY);
	if (filefd < 0)
	{
		_out("open fail");
		request_fail(connfd);
		pthread_exit(NULL);
	}

	int filesize = read(filefd , buf , 1024);
	close(filefd);
	buf [filesize] = '\0';
	headers(connfd , filename , strlen(buf));

	int statu = send(connfd , buf , filesize , 0);
	_out("send");

	pthread_exit(nullptr);
}



int main(int argc , char** argv)
{
	int listenfd = socket(AF_INET , SOCK_STREAM , 0);
	if (listenfd <0)
	{
		_out("socket create fail!");
		exit(0);
	}

	//允许重用本地地址
	int temp_flag = 1;
	if (setsockopt(listenfd , SOL_SOCKET , SO_REUSEADDR , &temp_flag , sizeof(temp_flag)) < 0)
	{
		_out("setsockopt failed");
	}
	//允许重用本地端口
	if (setsockopt(listenfd , SOL_SOCKET , SO_REUSEPORT , &temp_flag , sizeof(temp_flag)) < 0)
	{
		_out("setsockopt failed");
	}

	//sockaddr_in init here
	sockaddr_in servaddr;
	memset(&servaddr , 0 , sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(used_port);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(listenfd , (sockaddr*)&servaddr , sizeof(servaddr)) < 0)
	{
		_out("bind fail");
		exit(0);
	}

	if (listen(listenfd , 10) < 0)
	{
		_out("listen fail");
		exit(0);
	}

	while (true)
	{
		int connfd = accept(listenfd , nullptr , nullptr);
		pthread_t newthread;
		if (pthread_create(&newthread , NULL , (void*(*)(void*))handle_request ,(void*)(intptr_t) connfd) != 0)
		{
			_out("thread fail");
			exit(0);
		}
	}

	return 0;




}