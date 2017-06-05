#include"csapp.h"
#include<stdlib.h>
#include<stdio.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

int main()
{
	int fd1,fd2,fd3,fd4,fd5,fd6;
	int status;
	pid_t pid;
	char c;
	/* foo.txt 123456 */
	fd1 = open("foo.txt",O_RDONLY,0);
	fd2 = open("foo.txt",O_RDONLY,0);
	/* bar.txt abcdef */
	fd3 = open("bar.txt",O_RDWR,0);
	fd4 = open("alice.txt",O_WRONLY|O_CREAT|O_APPEND,S_IRUSR|S_IWUSR);
	fd5 = dup(STDOUT_FILENO);
	dup2(fd4,STDOUT_FILENO);
	if((pid = fork()) == 0){
		dup2(fd2,fd3);
		read(fd3,&c,1);printf("%c",c);
		write(fd4,&c,1);
		read(fd1,&c,1);printf("%c",c);
		read(fd2,&c,1);printf("%c",c);
		exit(0);
	}
	wait(NULL);
	read(fd1,&c,1);printf("%c",c);
	read(fd3,&c,1);printf("%c",c);
	dup2(fd5,STDOUT_FILENO);
	printf("done.\n");
	return 0;
}
