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
#include<sys/stat.h>
#include<sys/mman.h>
#include<sys/wait.h>
#include<sys/epoll.h>
#include"threadpool.hpp"

constexpr in_port_t used_port=80; // set port here
constexpr int MAX_EVENT_NUMBER=128;

#define SERVER_PATH "/your dir"

enum Method
{
	GET=0,
	POST,
	FAIL
};

inline int read_line(int connfd, char* usrbuf, int bufsize)
{
	int n=1;
	for (; n < bufsize; ++n)
	{
		recv(connfd, usrbuf, 1, 0);
		if ((*usrbuf=='\n')&&(*(usrbuf-1)=='\r'))
			break;
		++usrbuf;
	}
	++usrbuf;
	*usrbuf='\0';
	return n;
}

inline void pass_other(int connfd)
{
	char buf [256];
	while (read_line(connfd, buf, 256) > 2)
	{
	}
}

int get_content_length(int connfd)
{
	char buf [256];
	int ret;
	while (read_line(connfd, buf, 256) > 2)
	{
		std::string cont_len("Content-Length:");
		if (std::equal(cont_len.cbegin(), cont_len.cend(), buf))
		{
			ret=atoi(&(buf [15]));
		}
	}
	return ret;
}

void get_filetype(const char* filename, char* dest)
{
	if (strstr(filename, ".html"))
		strcpy(dest, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(dest, "image/gif");
	else if (strstr(filename, ".png"))
		strcpy(dest, "image/gif");
	else if (strstr(filename, ".jpg"))
		strcpy(dest, "image/gif");
	else
		strcpy(dest, "text/plain");
}

void bad_request(int connfd)
{
	char buf [1024];
	strcpy(buf, "HTTP/1.0 404 NOT FOUND\r\n");
	send(connfd, buf, strlen(buf), 0);
	strcpy(buf, "Content-Type: text/html\r\n");
	send(connfd, buf, strlen(buf), 0);
	sprintf(buf, "Content-length:%d\r\n", 64); //magic number
	send(connfd, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(connfd, buf, strlen(buf), 0);
	strcpy(buf, "<html><tile>Bad Request</title>\r\n");
	send(connfd, buf, strlen(buf), 0);
	strcpy(buf, "<body>400 bad request</body></html>\r\n");
	send(connfd, buf, strlen(buf), 0);
}

int headers(int connfd, const char* filename, int size)
{
	char buf [128];
	char filetype [128];
	strcpy(buf, "HTTP/1.0 200 OK\r\n");
	send(connfd, buf, strlen(buf), 0);
	strcpy(buf, "Server:Grt\r\n");
	send(connfd, buf, strlen(buf), 0);
	sprintf(buf, "Content-Type: %s\r\n", filetype);
	send(connfd, buf, strlen(buf), 0);
	sprintf(buf, "Content-length:%d\r\n", size);
	send(connfd, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(connfd, buf, strlen(buf), 0);
	return 0;
}

void request_fail(int connfd)
{
	char buf [1024];
	strcpy(buf, "HTTP/1.0 404 NOT FOUND\r\n");
	send(connfd, buf, strlen(buf), 0);
	strcpy(buf, "Content-Type: text/html\r\n");
	send(connfd, buf, strlen(buf), 0);
	sprintf(buf, "Content-length:%d\r\n", 62); //magic number
	send(connfd, buf, strlen(buf), 0);
	strcpy(buf, "\r\n");
	send(connfd, buf, strlen(buf), 0);
	strcpy(buf, "<html><tile>Not Found</title>\r\n");
	send(connfd, buf, strlen(buf), 0);
	strcpy(buf, "<body>404 not found</body></html>\r\n");
	send(connfd, buf, strlen(buf), 0);
}

void handle_cgi(int connfd, Method met, const char* filename, const char* args)
{
	char buf [256];
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	send(connfd, buf, strlen(buf), 0);

	if (met==GET)
	{
		pass_other(connfd);
		if (fork()==0)
		{
			setenv("QUERY_STRING", args, 1);
			dup2(connfd, STDOUT_FILENO); // equal to STDOUT
			execve(filename, NULL, environ);
		}
	}
	else if (met==POST)
	{
		int length=get_content_length(connfd);
		if (fork()==0)
		{
			char length_buf [16];
			sprintf(buf, "%s", length);
			setenv("REQUEST_METHOD", buf, 1);
			dup2(connfd, STDIN_FILENO);
			execve(filename, NULL, environ);
		}
	}
	wait(NULL);
	close(connfd);
}

void handle_request(void* arg)
{
	int connfd=(intptr_t)arg;

	char buf [1024];
	char method [100];
	char filename [100];
	char args [100];
	char* start;
	char* end;
	bool is_static=true;

	int rec_count=read_line(connfd, buf, 1024);
	end=std::find(buf, &buf [100], ' ');
	*(std::copy(buf, end, method))='\0'; //method
	start=end;
	++start;

	filename [0]='\0';
	strcat(filename, SERVER_PATH);
	end=std::find(start, &buf [100], '?');
	if (end!=(&buf [100]))
	{
		*(std::copy(start, end, filename+strlen(filename)))='\0';
		is_static=false;
		start=++end;
		end=std::find(start, &buf [100], ' ');
		if (end==(&buf [100]))
			end=start;
		*(std::copy(start, end, args))='\0';
	}
	else //can't find '?'
	{
		end=std::find(start, &buf [100], ' ');
		*(std::copy(start, end, filename+strlen(filename)))='\0';
	}
	if (*(end-1)=='/')
	{
		strcat(filename, "index.html");
	}

	if (is_static)
	{
		pass_other(connfd);
		int filefd=open(filename, O_RDONLY);
		if (filefd < 0)
		{
			request_fail(connfd);
			return;
		}

		struct stat file_stat;
		fstat(filefd, &file_stat);
		int filesize=file_stat.st_size;
		void* file_start=mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, filefd, 0);

		close(filefd);
		buf [filesize]='\0';
		headers(connfd, filename, filesize);

		send(connfd, file_start, filesize, 0);
	}
	else // CGI
	{
		Method met;
		if (strcasecmp(method, "GET")==0)
		{
			met=GET;
		}
		else if (strcasecmp(method, "POST")==0)
		{
			met=POST;
		}
		else
		{
			bad_request(connfd);
		}
		handle_cgi(connfd, met, filename, args);
	}
}

inline void epoll_addfd(int epollfd, int fd)
{
	epoll_event eevent;
	eevent.data.fd=fd;
	eevent.events=EPOLLIN;
	if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &eevent)==-1)
	{
		exit(0);
	}
}

inline void epoll_delfd(int epollfd, int fd)
{
	epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

class work
{
public:
	explicit work(int connfd)
		:connfd_(connfd)
	{   }

	void run()
	{
		handle_request((void*)(intptr_t)connfd_);
	}
private:
	int connfd_;
};


int main(int argc, char** argv)
{
	int listenfd=socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd <0)
	{
		exit(0);
	}

	int temp_flag=1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &temp_flag, sizeof(temp_flag)) < 0)
	{
		exit(0);
	}

	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEPORT, &temp_flag, sizeof(temp_flag)) < 0)
	{
		exit(0);
	}

	//sockaddr_in init here
	sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family=AF_INET;
	servaddr.sin_port=htons(used_port);
	servaddr.sin_addr.s_addr=htonl(INADDR_ANY);

	if (bind(listenfd, (sockaddr*)&servaddr, sizeof(servaddr)) < 0)
	{
		exit(0);
	}

	if (listen(listenfd, 100) < 0)
	{
		exit(0);
	}

	//threadpool init
	threadpool<work> pool(8, 1000);

	int epfd=epoll_create(128);
	epoll_addfd(epfd, listenfd);
	epoll_event events [MAX_EVENT_NUMBER];

	for(;;)
	{
		int num=epoll_wait(epfd, events, MAX_EVENT_NUMBER, -1);
		if (num<0)
		{
			exit(0);
		}

		for (int i=0; i<num; ++i)
		{
			int eventfd=events [i].data.fd;
			if (eventfd==listenfd)
			{
				int connfd=accept(listenfd, NULL, NULL);
				epoll_addfd(epfd, connfd);
			}
			else if(events[i].events & EPOLLIN)
			{
				pool.append(std::make_shared<work>(eventfd));
				epoll_delfd(epfd, eventfd);
			}
			else
			{
				continue;
			}
		}
	}

	return 0;
}

