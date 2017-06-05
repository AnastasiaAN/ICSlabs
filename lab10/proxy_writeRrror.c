/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     Andrew Carnegie, ac00@cs.cmu.edu 
 *     Harry Q. Bovik, bovik@cs.cmu.edu
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 

#include "csapp.h"
#include <stdio.h>
#include<stdlib.h>

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
int Rio_writen_w(int fd, void *usrbuf, size_t n);
void doit(int fd,struct sockaddr_in* sockaddr);  
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);  

FILE* logFile;

ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n) {
    ssize_t rc;
	printf("readnb\n");
    if ((rc = rio_readnb(rp, usrbuf, n)) < 0) {
        printf("Rio_readnb error\n");
        return 0;
    }
	printf("readnb ok\n");
    return rc;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen) {
    ssize_t rc;
    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        printf("Rio_readlineb error\n");
        return 0;
    }
    return rc;
} 

int Rio_writen_w(int fd, void *usrbuf, size_t n) 
{
	int words;
    if ((words = rio_writen(fd, usrbuf, n) )!= n) {
        printf("Rio_writen error\n");
        return 0;
    }

	printf("writen:%d\n", words);
    return words;
}
  
int main(int argc, char **argv)  
{  
    int listenfd, connfd, port;
	socklen_t clientlen;  
    struct sockaddr_in clientaddr;  
    logFile = fopen("proxy.log", "w");
	if(logFile == NULL)
	{
		printf("log fails\n");
	}
	else
	{
		printf("log success\n");
	}
    /* Check arguments */  
    if (argc != 2)  
    {  
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);  
        exit(0);  
    }  

    port = atoi(argv[1]);  
  
    listenfd = Open_listenfd(port);  
    while (1)  
    {  
        clientlen = sizeof(clientaddr);  
        connfd = Accept(listenfd,  (SA *)&clientaddr, &clientlen);  
        doit(connfd,&clientaddr);  
        Close(connfd);  
    }  
    exit(0);
}  
  
void doit(int fd, struct sockaddr_in* sockaddr)  
{  
    int proxyfd;  
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE]; 
    char serverHost[MAXLINE], serverPath[MAXLINE];  
	int serverPort;	
    rio_t clientRio, serverRio;  
  
    /* Read request line and headers */  
    Rio_readinitb(&clientRio, fd);  
    Rio_readlineb_w(&clientRio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);  
	memset(version,0,sizeof(version));
	strcpy(version,"HTTP/1.0");
    if (strcasecmp(method, "GET"))  
    {  
        clienterror(fd, method, "501", "Not Implemented",  
                "Tiny does not implement this method");  
        return ;  
    }  
  
    /* Parse URI from GET request */
	if (parse_uri(uri, serverHost, serverPath, &serverPort) < 0) {
		//printf("URI Error!\n");
		return;
	}
	sprintf(buf, "%s /%s %s\r\n", method, serverPath, version);

	//printf("version:%s\n",version);
	//printf("buf:%s\n",buf);

	proxyfd=Open_clientfd(serverHost,serverPort);
	//printf("line:%s\n",buf);
	//printf("Host:%s\n",serverHost);
	/* send request line in buf */
	if(Rio_writen_w(proxyfd,buf,strlen(buf))<0)
	{
		Close(proxyfd);
		return;
	}

	/*memset(buf,0,sizeof(buf));
	sprintf(buf, "%s %s","Host:",serverHost);
	printf("buf:%s\n",buf);
	if(Rio_writen_w(proxyfd,buf,strlen(buf))<0)
	{
		Close(proxyfd);
		return;
	}*/
	/* read request header from client and send it to server */
	
	while(strcmp(buf,"\r\n"))
	{
		Rio_readlineb_w(&clientRio,buf,MAXLINE);
		//printf("head%s\n",buf);	
		
		if(Rio_writen_w(proxyfd,buf,strlen(buf)) == -1)
		{
			//printf("mistakes\n");
			Close(proxyfd);
			return;
		}
		
		//printf("after write\n");
	}
	//printf("before init\n");
	Rio_readinitb(&serverRio, proxyfd);
	int n,size = 0;
	while((n = Rio_readlineb_w(&serverRio, buf, MAXLINE)) > 0)
	{
		printf("response:%s\n",buf);	
		if (Rio_writen_w(fd, buf, n) != n)
		{
			break;
		}
		size += n;
		printf("now size: %d\n", size);
	}
	
	printf("send to client finish\n");
	format_log_entry(buf,sockaddr,uri,size);
	fprintf(logFile,"%s\n",buf);
	fflush(logFile);
	Close(proxyfd);

}  
  
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)  
{  
    char buf[MAXLINE], body[MAXBUF];  
  
    /* Build the HTTP response body */  
    sprintf(body, "<html><title>Tiny Error</title>");  
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\bn", body);  
    sprintf(body, "%s%s: %s\r\n",body, errnum, shortmsg);  
    sprintf(body, "%s<p>%s:%s\r\n", body, longmsg, cause);  
    sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);  
  
    /* Print the HTTP response */  
    sprintf(buf, "HTTP 1.0 %s %s \r\n", errnum, shortmsg);  
    Rio_writen(fd, buf, strlen(buf));  
    sprintf(buf, "Content-type:text/html\r\n");  
    Rio_writen(fd, buf, strlen(buf));  
    sprintf(buf, "content-lengt:%d\r\n\r\n", (int)strlen(body));  
    Rio_writen(fd, buf, strlen(buf));  
    Rio_writen(fd, body, strlen(body));  
}  
  


/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    int len;

    if (strncasecmp(uri, "http://", 7) != 0) {
	hostname[0] = '\0';
	return -1;
    }
       
    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk(hostbegin, " :/\r\n\0");
    len = hostend - hostbegin;
    strncpy(hostname, hostbegin, len);
    hostname[len] = '\0';
    
    /* Extract the port number */
    *port = 80; /* default */
    if (*hostend == ':')   
	*port = atoi(hostend + 1);
    
    /* Extract the path */
    pathbegin = strchr(hostbegin, '/');
    if (pathbegin == NULL) {
	pathname[0] = '\0';
    }
    else {
	pathbegin++;	
	strcpy(pathname, pathbegin);
    }
    return 0;
}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
		      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %d\n", time_str, a, b, c, d, uri, size);
}


