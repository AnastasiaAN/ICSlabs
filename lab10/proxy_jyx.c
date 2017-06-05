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
 *
 * This tiny proxy implements a simple connection between the client
 * and the server. It first receives the request header and the request
 * body from the client and sends it to the server. Then, it receives the
 * response body and header from the server and sends it to the client.
 *
 * This proxy support concurrency, using thread technique. It is tiny and it
 * might not be so robust, but it is ok to use.
 */ 


/*
 * Name: Jin Yongxu
 * Student ID: 515030910286
 * Email: dowbee@sjtu.edu.cn
 */

#include "csapp.h"

#define MIN(a, b) (a < b) ? a : b
#define MAX(a, b) (a > b) ? a : b

typedef struct{
    int clientfd;
    struct sockaddr_in clientaddr;
} conn_t;

/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void proxy(int clientfd, struct sockaddr_in clientaddr);
void *thread(void *vargp);
int open_clientfd_ts(char *hostname, int port);
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen);
int Rio_writen_w(int fd, void *usrbuf, size_t n);

FILE *logfd;
sem_t log_mutex;
sem_t clientfd_mutex;

/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
    int listenfd, port;
    socklen_t clientlen;
    pthread_t tid;
    conn_t *conn;

    /* Check arguments */
    if (argc != 2) {
	    fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
	    exit(0);
    }

    port = atoi(argv[1]);
    listenfd = Open_listenfd(port);

    /* Init the semaphore */
    Sem_init(&log_mutex, 0, 1);
    Sem_init(&clientfd_mutex, 0, 1);

    /* Ignore SIGPIPE */
    Signal(SIGPIPE, SIG_IGN);

    /* Open the log file */
    logfd = Fopen("proxy.log", "w");

    while(1){
        conn = Malloc(sizeof(conn_t));
        clientlen = sizeof(conn->clientaddr);
        conn->clientfd = Accept(listenfd, (SA *)&(conn->clientaddr), &clientlen);
        Pthread_create(&tid, NULL, thread, conn);
    }

    Close(listenfd);
    Fclose(logfd);

    exit(0);
}

/*
 * thread - this function process each thread
 */
void *thread(void *vargp)
{
    conn_t *conn = (conn_t *)vargp;
    Pthread_detach(pthread_self());
    int connfd = conn->clientfd;
    struct sockaddr_in clientaddr = conn->clientaddr;
    Free(vargp);
    proxy(connfd, clientaddr);
    Close(connfd);
    return NULL;
}

/*
 * proxy - handle the proxy function
 */
void proxy(int clientfd, struct sockaddr_in clientaddr)
{
    rio_t client_rio, server_rio;
    char buf[MAXLINE], tmp[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char hostname[MAXLINE], pathname[MAXLINE];
    int port, serverfd, n, left;
    int size = -1;

    /* Init the request stream sent by the client, and parse the URI */
    Rio_readinitb(&client_rio, clientfd);
    Rio_readlineb_w(&client_rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    parse_uri(uri, hostname, pathname, &port);

    /* Init the server side response stream */
    serverfd = open_clientfd_ts(hostname, port);
    Rio_readinitb(&server_rio, serverfd);

    /* Send the request header (using HTTP/1.0 protocol) */
    sprintf(buf, "%s /%s HTTP/1.0\r\n", method, pathname);
    Rio_readlineb_w(&client_rio, tmp, MAXLINE);
    while(strcmp(tmp, "\r\n")) {
        strcat(buf, tmp);
        Rio_readlineb_w(&client_rio, tmp, MAXLINE);
        if (strncasecmp(tmp, "Content-length:", 15) == 0)
            sscanf(tmp + 15, "%d", &size);
    }
    sprintf(buf, "%s\r\n\r\n", buf);
    Rio_writen_w(serverfd, buf, strlen(buf));
    memset(buf, 0, sizeof(buf));

    /* Send the request body if there has one */
    if (size > 0){
        left = size;
        n = MIN(left, MAXLINE);
        while ((n = Rio_readnb_w(&client_rio, tmp, n)) != 0) {
            Rio_writen_w(serverfd, tmp, n);
            left = MAX(left - n, 0);
            n = MIN(left, MAXLINE);
        }
        Rio_writen_w(serverfd, "\r\n", 2);
    }

    size = -1;
    /* Receive response header */
    Rio_readlineb_w(&server_rio, tmp, MAXLINE);
    while (strcmp(tmp, "\r\n")){
        strcat(buf, tmp);
        Rio_readlineb_w(&server_rio, tmp, MAXLINE);
        if (strncasecmp(tmp, "Content-length:", 15) == 0) {
            sscanf(tmp + 15, "%d", &size);
        }
    }
    Rio_writen_w(serverfd, buf, strlen(buf));

    /* Receive response body */
    if (size > 0){
        left = size;
        n = MIN(left, MAXLINE);
        while ((n = Rio_readnb_w(&server_rio, tmp, n)) != 0) {
            Rio_writen_w(clientfd, tmp, n);
            left = MAX(left - n, 0);
            n = MIN(left, MAXLINE);
        }
    }
    else if (size == -1){
        size = 0;
        while ((n = Rio_readnb_w(&server_rio, tmp, MAXLINE)) != 0){
            Rio_writen_w(clientfd, tmp, n);
            size += n;
        }
    }

    Close(serverfd);

    /* Using thread safe technique to write the log file */
    format_log_entry(buf, &clientaddr, uri, size);
    P(&log_mutex);
    Fwrite(buf, strlen(buf), 1, logfd);
    fflush(logfd);
    V(&log_mutex);

    return;
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


/*
 * open_clientfd_ts - thread safe version of open_client_fd
 */
int open_clientfd_ts(char *hostname, int port)
{
    int clientfd;
    struct hostent *hp, *thp;
    struct sockaddr_in serveraddr;

    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    
    /* Using semaphore to protect gethostbyname */
    P(&clientfd_mutex);
    if ((thp = gethostbyname(hostname)) == NULL)
        return -2;
    hp = Malloc(sizeof(struct hostent));
    memcpy(hp, thp, sizeof(struct hostent));
    V(&clientfd_mutex);

    bzero((char *)&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0],
          (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    if (connect(clientfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0)
        return -1;
    return clientfd;
}

/* The following three functions are wrappers of rio functions that return normally when I/O fails */
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n)
{
    ssize_t size;
    if ((size = rio_readnb(rp, usrbuf, n)) < 0){
        printf("Rio_readnb ERROR\n");
        return 0;
    }
    return size;
}

ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen)
{
    ssize_t size;
    if ((size = rio_readlineb(rp, usrbuf, maxlen)) < 0){
        printf("Rio_readlineb ERROR\n");
        return 0;
    }
    return size;
}

int Rio_writen_w(int fd, void *usrbuf, size_t n)
{
    if (rio_writen(fd, usrbuf, n) != n){
        printf("Rio_writen ERROR\n");
        return -1;
    }
    return 0;
}