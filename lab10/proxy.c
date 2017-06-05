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

/*
 * Name:Luo Anqi
 * Student Number:515030910431
 * Email:miracledestiny@sjtu.edu.cn
 */
#include "csapp.h"

/* struct for thread args */
typedef struct {
    int clientfd;
    struct sockaddr_in clientaddr;
} conn_t;

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
ssize_t Rio_writen_w(int fd, void *usrbuf, size_t n);
void doit(int clientfd,struct sockaddr_in* sockaddr);  
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);  
int open_clientfd_ts(char *hostname, int port);
void *thread(void *vargp);

FILE* logFile;
sem_t log_mutex;
sem_t fd_mutex;

/* wrapped rio_readnb that simply return after printing a warning message when I/O fails */
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n) {
    ssize_t rc;
    if ((rc = rio_readnb(rp, usrbuf, n)) < 0) {
        printf("Rio_readnb error\n");
        return 0;
    }
    return rc;
}

/* wrapped rio_readlineb that simply return after printing a warning message when I/O fails */
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen) {
    ssize_t rc;
    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0) {
        printf("Rio_readlineb error\n");
        return 0;
    }
    return rc;
} 

/* wrapped rio_writen that simply return after printing a warning message when I/O fails */
ssize_t Rio_writen_w(int fd, void *usrbuf, size_t n) 
{
	ssize_t words;
    if ((words = rio_writen(fd, usrbuf, n) )!= n) {
        printf("Rio_writen error\n");
        return -1;
    }
    return 0;
}

/* Thread routine */ 
void *thread(void *vargp)
{
    conn_t *conn = (conn_t *)vargp;
    Pthread_detach(pthread_self());
    int connfd = conn->clientfd;
    struct sockaddr_in clientaddr = conn->clientaddr;
    Free(vargp);
    doit(connfd, &clientaddr);
    Close(connfd);
    return NULL;
}

/* Main routine */
int main(int argc, char **argv)  
{  
    int listenfd, port;
	socklen_t clientlen;  
	pthread_t tid;
    conn_t *conn;
	/* Init the semaphore */
    Sem_init(&log_mutex, 0, 1);
    Sem_init(&fd_mutex, 0, 1);

    logFile = fopen("proxy.log", "w");

	Signal(SIGPIPE,SIG_IGN);

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
		conn = Malloc(sizeof(conn_t));
        clientlen = sizeof(conn->clientaddr);
        conn->clientfd = Accept(listenfd, (SA *)&(conn->clientaddr), &clientlen);
        Pthread_create(&tid, NULL, thread, conn); 
    }  
    Close(listenfd);
    Fclose(logFile);
    exit(0);
}  

/* 
 * acting as a proxy
 * receive request from client and send it to server
 * then receive response from server and send it to client
 */
void doit(int fd, struct sockaddr_in* sockaddr)  
{  
    int proxyfd;  
    char buf[MAXLINE], tmp[MAXLINE],method[MAXLINE], uri[MAXLINE], version[MAXLINE]; 
    char serverHost[MAXLINE], serverPath[MAXLINE];  
	int serverPort;	
    rio_t clientRio, serverRio;  
  
    /* Read request line and headers */  
    Rio_readinitb(&clientRio, fd);  
    Rio_readlineb_w(&clientRio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);  
	/* Change to HTTP 1.0 */
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
		printf("URI Error!\n");
		return;
	}
	sprintf(buf, "%s /%s %s\r\n", method, serverPath, version);

	/* thread-safe open */
	proxyfd=open_clientfd_ts(serverHost,serverPort);

	/* send request line in buf */
	if(Rio_writen_w(proxyfd,buf,strlen(buf))<0)
	{
		Close(proxyfd);
		return;
	}


	/* read rest part of request and send it */	
	while(strcmp(buf,"\r\n"))
	{
		Rio_readlineb_w(&clientRio,buf,MAXLINE);
		
		if(Rio_writen_w(proxyfd,buf,strlen(buf)) == -1)
		{
			Close(proxyfd);
			return;
		}

	}
	
	Rio_readinitb(&serverRio, proxyfd);
	
	/* Receive response header and send it*/
	Rio_readlineb_w(&serverRio, tmp, MAXLINE);
	while (strcmp(tmp, "\r\n"))
	{
		strcat(buf, tmp);
		Rio_readlineb_w(&serverRio, tmp, MAXLINE);
	}
	Rio_writen_w(proxyfd, buf, strlen(buf));

	/* Receive response body and send it*/
	int n,size = 0;
	while((n = Rio_readnb_w(&serverRio, buf, MAXLINE)) > 0)
	{
		rio_writen(fd, buf, n);
		size += n;
	}

	Close(proxyfd);
    /* Using thread safe technique to write the log file */
    format_log_entry(buf,sockaddr,uri,size);
    P(&log_mutex);
	fprintf(logFile,"%s",buf);
	fflush(logFile);
    V(&log_mutex);

    return;

}  

/* thread-safe Open-clientfd */
int open_clientfd_ts(char *hostname, int port)
{
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    
    /* Using semaphore to protect gethostbyname */
    P(&fd_mutex);
    if ((hp = gethostbyname(hostname)) == NULL)
	{
	V(&fd_mutex);
        return -2;
	}

    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0],
          (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    V(&fd_mutex);

    serveraddr.sin_port = htons(port);

    if (connect(clientfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    return clientfd;
}

/* error handler */
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


