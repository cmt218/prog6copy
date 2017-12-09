#include <thread>
#include <mutex>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/md5.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <unistd.h>
#include "support.h"
#include "Server.h"


struct LockGuy {

};

struct Cache {

};

struct Node {
	char *fname;
	char *fcontents;
	int lru_index;
};

int numelements(int lru_size, Node **mycache){
	int numberofelements = 0;
	int counter = 0;
	for(counter; counter < lru_size; counter++){
		if(mycache[counter] != NULL){
			numberofelements += 1;
		}
	}
	return numberofelements;
}

void remove(int lru_size, Node **mycache){

	//still need to do
}

void addNode(char *contents, char *name, int lru_size, Node **mycache){

	struct Node *mynode = (Node*)malloc(sizeof(struct Node));
	mynode->fname = (char*)malloc(strlen(name));
	mynode->fcontents = (char*)malloc(strlen(contents));
	strcpy(mynode->fname, name);
	strcpy(mynode->fcontents, contents);
	mynode->lru_index = 1;
	int inc2;
	for(inc2=0; inc2 < lru_size; inc2++){
		//increase the non-null elements lru-index by 1
		int temp = mycache[inc2]->lru_index;
		mycache[inc2]->lru_index = temp+1;
	}
	if(lru_size == numelements(lru_size, mycache)){

		remove(lru_size, mycache);
	}
	int inc;
	for(inc=0;inc<lru_size; inc++){

		if(mycache[inc] == NULL){
			mycache[inc] = mynode;
			return;
		}
	}
}

void help(char *progname)
{
	printf("Usage: %s [OPTIONS]\n", progname);
	printf("Initiate a network file server\n");
	printf("  -m    enable multithreading mode\n");
	printf("  -l    number of entries in the LRU cache\n");
	printf("  -p    port on which to listen for connections\n");
}

void die(const char *msg1, char *msg2)
{
	fprintf(stderr, "%s, %s\n", msg1, msg2);
	exit(0);
}

/*
 * open_server_socket() - Open a listening socket and return its file
 *                        descriptor, or terminate the program
 */
int open_server_socket(int port)
{
	int                listenfd;    /* the server's listening file descriptor */
	struct sockaddr_in addrs;       /* describes which clients we'll accept */
	int                optval = 1;  /* for configuring the socket */

	/* Create a socket descriptor */
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		die("Error creating socket: ", strerror(errno));
	}

	/* Eliminates "Address already in use" error from bind. */
	if(setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int)) < 0)
	{
		die("Error configuring socket: ", strerror(errno));
	}

	/* Listenfd will be an endpoint for all requests to the port from any IP
	   address */
	bzero((char *) &addrs, sizeof(addrs));
	addrs.sin_family = AF_INET;
	addrs.sin_addr.s_addr = htonl(INADDR_ANY);
	addrs.sin_port = htons((unsigned short)port);
	if(bind(listenfd, (struct sockaddr *)&addrs, sizeof(addrs)) < 0)
	{
		die("Error in bind(): ", strerror(errno));
	}

	/* Make it a listening socket ready to accept connection requests */
	if(listen(listenfd, 1024) < 0)  // backlog of 1024
	{
		die("Error in listen(): ", strerror(errno));
	}

	return listenfd;
}

/*
 * handle_requests() - given a listening file descriptor, continually wait
 *                     for a request to come in, and when it arrives, pass it
 *                     to service_function.  Note that this is not a
 *                     multi-threaded server.
 */
void handle_requests(int listenfd, void (*service_function)(int, int, struct LockGuy*), int param, bool multithread)
{
	while(1)
	{
		/* block until we get a connection */
		struct sockaddr_in clientaddr;
		memset(&clientaddr, 0, sizeof(sockaddr_in));
		socklen_t clientlen = sizeof(clientaddr);
		int connfd;
		if((connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientlen)) < 0)
		{
			die("Error in accept(): ", strerror(errno));
		}

		/* print some info about the connection */
		struct hostent *hp;
		hp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if(hp == NULL)
		{
			fprintf(stderr, "DNS error in gethostbyaddr() %d\n", h_errno);
			exit(0);
		}
		char *haddrp = inet_ntoa(clientaddr.sin_addr);
		printf("server connected to %s (%s)\n", hp->h_name, haddrp);


		if(multithread){
			LockGuy * lg = new LockGuy();
			/* serve requests */
			service_function(connfd, param, lg);
		}else{
			service_function(connfd, param, NULL);
		}

		/* clean up, await new connection */
		if(close(connfd) < 0)
		{
			die("Error in close(): ", strerror(errno));
		}
	}
}

/*
 * file_server() - Read a request from a socket, satisfy the request, and
 *                 then close the connection.
 */
void file_server(int connfd, int lru_size, struct LockGuy* lg)
{
	/* TODO: set up a few static variables here to manage the LRU cache of
	   files */
	static struct Node **mycache;
	if(lru_size > 0){

		mycache = ((Node**)malloc(lru_size * ((sizeof(struct Node)) + sizeof(char *) + sizeof(char * ) + sizeof(int))));
	}


	while(1)
	{
		const int MAXLINE = 8192;
		char      buf[MAXLINE];   /* a place to store text from the client */
		bzero(buf, MAXLINE);

		/* read from socket, recognizing that we may get short counts */
		char *bufp = buf;              /* current pointer into buffer */
		ssize_t nremain = MAXLINE;     /* max characters we can still read */
		size_t nsofar;                 /* characters read so far */
		while (1)
		{
			/* read some data; swallow EINTRs */
			if((nsofar = read(connfd, bufp, nremain)) < 0)
			{
				if(errno != EINTR)
				{
					die("read error: ", strerror(errno));
				}
				continue;
			}
			/* end service to this client on EOF */
			if(nsofar == 0)
			{
				fprintf(stderr, "received EOF\n");
				return;
			}
			/* update pointer for next bit of reading */
			bufp += nsofar;
			nremain -= nsofar;
			
			if(*(bufp-1) == '\n')
			{
				*bufp = 0;
				break;
			}
		}

		if(strncmp(buf, "PUTC", 4) == 0){
			putc_file(buf, lru_size, mycache, lg);
			continue;
		}

		else if(strncmp(buf, "GETC", 4) == 0){
			getc_file(connfd, buf, lg);
			continue;
		}

		//case where server receives put
		else if(strncmp(buf, "PUT", 3) == 0){
			put_file(buf, lg);
			continue;
		}
		
		//case where server receives get
		else if(strncmp(buf, "GET", 3) == 0){
			get_file(connfd, buf, lg);
			continue;
		}

		/* dump content back to client (again, must handle short counts) */
		printf("server received %d bytes\n", MAXLINE-nremain);

		//fprintf(stderr, "BUF CONTENTS SERVER a: %s\n", buf);

		nremain = bufp - buf;
		bufp = buf;
		while(nremain > 0)
		{
			/* write some data; swallow EINTRs */
			if((nsofar = write(connfd, bufp, nremain)) <= 0)
			{
				if(errno != EINTR)
				{
					die("Write error: ", strerror(errno));
				}
				nsofar = 0;
			}
			nremain -= nsofar;
			bufp += nsofar;
		}
	}
}



//receives:
//PUT <filename>\n
//<# bytes>\n
//<file contents>\n
void put_file(char* putmsg, struct LockGuy* lg){
	
	//parse out file name
	char* endname = strstr(putmsg, "\n");
	char* begname = putmsg+4;
	int len = endname-begname;
	char filename[len+1];
	bzero(filename, len+1);
	strncpy(filename, begname, len);
	filename[len+1] = '\0';
	
	//parse out bytes size
	begname = endname+1;
	endname = strstr(begname, "\n");
	len = endname-begname;
	int numbytes;
	char numbytesstring[len+1];
	bzero(numbytesstring, len+1);
    strncpy(numbytesstring, begname, len);
	numbytesstring[len+1] = '\0';
	sscanf(numbytesstring, "%d", &numbytes);

	//isolate the file data
	begname = endname+1;
	//endname = strstr(begname, "\n");
	endname = begname+numbytes;
	len = endname-begname;
	char filedata[len+1];
	bzero(filedata, len+1);
	strncpy(filedata, begname, len);
	filedata[len] = '\0';

	//write file
	int writefd = open(filename, O_RDWR | O_TRUNC | O_CREAT, 0777);
	if(lg){
		flock(writefd, LOCK_EX);
	}
	write(writefd, filedata, len);
	close(writefd);
	if(lg){
		flock(writefd, LOCK_UN);
	}
	printf("OK\n");
}

//receives:
//PUTC <filename>\n
//<checksum>\n
//<# bytes>\n
//<file contents>\n
void putc_file(char* putmsg, int lru_size, Node **mycache, struct LockGuy* lg){

	//parse out file name
	char* endname = strstr(putmsg, "\n");
	char* begname = putmsg+5;
	int len = endname-begname;
	char filename[len+1];
	bzero(filename, len+1);
	strncpy(filename, begname, len);
	filename[len+1] = '\0';

	//parse out bytes size
	begname = endname+1;
	endname = strstr(begname, "\n");
	len = endname-begname;
	int numbytes;
	char numbytesstring[len+1];
	bzero(numbytesstring, len+1);
    strncpy(numbytesstring, begname, len);
	numbytesstring[len+1] = '\0';
	sscanf(numbytesstring, "%d", &numbytes);

	//parse out the checksum
	begname = endname+1;
	endname = strstr(begname, "\n");
	len = endname-begname;
	char checksum[len+1];
	bzero(checksum, len+1);
	strncpy(checksum, begname, len);
	checksum[len+1] = '\0';

	//isolate the file data
	begname = endname+1;
	endname = begname+numbytes;
	len = endname-begname;
	char filedata[len+1];
	bzero(filedata, len+1);
	strncpy(filedata, begname, len);
	filedata[len] = '\0';
		
	//see if the checksum matches
	unsigned char digest[16];
	char* data = filedata;
	int read_bytes;
	MD5_CTX context;
	MD5_Init(&context);
	MD5_Update(&context, data, len-1);

	MD5_Final(digest, &context);

	char md5string[32];
	for(int i=0; i<16; i++){
		sprintf(md5string, "%02x", digest[i]);
		if(strncmp(md5string, checksum+(2*i), 2) == 0){
			continue;
		}
		else{
			printf("non matching checksums\n");
			return;
		}
	}
	
	//write file 
	int writefd = open(filename, O_RDWR | O_TRUNC | O_CREAT, 0777);
	if(lg){
		fprintf(stderr, "ACQUIRING LOCK PUT \n");
		flock(writefd, LOCK_EX);
	}
	write(writefd, filedata, len);
	close(writefd);
	if(lg){
		fprintf(stderr, "FREEING LOCK PUT \n");
		flock(writefd, LOCK_UN);
	}
	printf("OK\n");
}


//receives:
//GET <filename>\n
 void get_file(int connfd, char* get_msg, struct LockGuy* lg){

 	//parse message and error check
 	char* endname = strstr(get_msg, "\n");
 	char* begname = get_msg+4;
 	int len = endname - begname;
 	char filename[len+1];
 	bzero(filename, len+1);
 	strncpy(filename, begname, len);
 	filename[len+1] = '\0';

 	//<error>\n
 	if(!file_exists(filename)){
 		write(connfd, "file not found\n", 15);
 		return;
 	}

 	//sends:
 	//OK <filename>\n
 	//<# bytes>\n
 	//<file contents>\n
 	FILE *newptr;
 	newptr = fopen(filename, "r");
 	int fd = fileno(newptr);
 	if(lg){
		flock(fd, LOCK_EX);
	}

 	//calculate message size
	size_t sendmsgsize = 3;
	sendmsgsize += sizeof(char*)*strlen(filename);
	size_t sendfilesize = get_size(filename);
	sendmsgsize += digitcount(sendfilesize);
	sendmsgsize += sendfilesize;
	char sendmsg[sendmsgsize];
	bzero(sendmsg, sendmsgsize);

	//OK <filename>\n
	strcat(sendmsg, "OK ");
	strcat(sendmsg, filename);
	strcat(sendmsg, "\n");

	//<#bytes>\n
	char sizestr[digitcount(sendfilesize)];
	sprintf(sizestr, "%d", sendfilesize);
	strcat(sendmsg, sizestr);
	strcat(sendmsg, "\n");

	//<file contents>\n
	char* contentstr = (char*)malloc(sizeof(char*)*sendfilesize);
	for(int i=0;i<sendfilesize;i++){
		fread(contentstr+i, 1, 1, newptr);
	}
	strcat(sendmsg, contentstr);
	strcat(sendmsg, "\n");
	write(connfd, sendmsg, strlen(sendmsg));
	if(lg){
		flock(fd, LOCK_UN);
	}
 }

//receives:
//GETC <filename>\n
 void getc_file(int connfd, char* get_msg, struct LockGuy* lg){
 	
 	//parse message and error check
 	char* endname = strstr(get_msg, "\n");
 	char* begname = get_msg+5;
 	int len = endname - begname;
 	char filename[len+1];
 	bzero(filename, len+1);
 	strncpy(filename, begname, len);
 	filename[len+1] = '\0';
 	
 	//<error>\n
 	if(!file_exists(filename)){
 		write(connfd, "file not found\n", 15);
 		return;
 	}

 	//sends:
 	//OKC <filename>\n
 	//<checksum>\n
 	//<# bytes>\n
 	//<file contents>\n
 	FILE *newptr;
 	newptr = fopen(filename, "r");
 	int fd = fileno(newptr);
 	if(lg){
 		fprintf(stderr, "ACQUIRING LOCK GET \n");
		flock(fd, LOCK_EX);
	}

	//calculate message size
	size_t sendmsgsize = 4;
	sendmsgsize += sizeof(char*)*strlen(filename);
	size_t sendfilesize = get_size(filename);
	sendmsgsize += digitcount(sendfilesize);
	sendmsgsize += sendfilesize;
	char sendmsg[sendmsgsize];
	bzero(sendmsg, sendmsgsize);

	//OKC <filename>\n
	strcat(sendmsg, "OKC ");
	strcat(sendmsg, filename);
	strcat(sendmsg, "\n");

	//<# bytes>\n
	char sizestr[digitcount(sendfilesize)];
	sprintf(sizestr,"%d",sendfilesize);
	strcat(sendmsg, sizestr);
	strcat(sendmsg, "\n");
	
	//get file contents
	char* contentstr = (char*)malloc(sizeof(char*)*sendfilesize);
	for(int i=0;i<sendfilesize;i++){
		fread(contentstr+i, 1, 1, newptr);
	}
	rewind(newptr);

	//get checksum
	unsigned char digest[16];
	char* data = contentstr;
	int read_bytes;
	MD5_CTX context;
	MD5_Init(&context);
    MD5_Update(&context, data, sendfilesize-1);
	MD5_Final(digest, &context);

	//<checksum\n
	char md5string[32];
	for(int i=0; i<16; i++){
		sprintf(md5string, "%02x", digest[i]);
		strcat(sendmsg, md5string);
	}
	strcat(sendmsg, "\n");

	//<file contents>\n
	strcat(sendmsg, contentstr);
	strcat(sendmsg, "\n");
	write(connfd, sendmsg, strlen(sendmsg));
	if(lg){
		fprintf(stderr, "FREEING LOCK GET \n");
		flock(fd, LOCK_UN);
	}
 }

 int digitcount(int num){
 	int cpy = num;
 	int ct = 0;
 	while(cpy != 0){
 		cpy/=10;
 		ct++;
 	}
 	return ct;
 }

/*cmt218
 *file_exists() - check if a file exists in the current directory
 *
 */
bool file_exists(char *name){
	if(FILE *test = fopen(name, "r")){
		fclose(test);
		return true;
	}
	return false;
}

/*cmt218
 *get_size() - return size of a file in bytes
 *
 */
size_t get_size(char *name){
	FILE *getsizeof = fopen(name, "r");
	fseek(getsizeof, 0, SEEK_END);
	size_t size = ftell(getsizeof);
	rewind(getsizeof);
	fclose(getsizeof);
	return size;
}

/*
 * main() - parse command line, create a socket, handle requests
 */
int main(int argc, char **argv)
{
	/* for getopt */
	long opt;
	int  lru_size = 10;
	int  port     = 9000;
	bool multithread = false;

	check_team(argv[0]);

	/* parse the command-line options.  They are 'p' for port number,  */
	/* and 'l' for lru cache size, 'm' for multi-threaded.  'h' is also supported. */
	while((opt = getopt(argc, argv, "hml:p:")) != -1)
	{
		switch(opt)
		{
		case 'h': help(argv[0]); break;
		case 'l': lru_size = atoi(optarg); break;
		case 'm': multithread = true;	break;
		case 'p': port = atoi(optarg); break;
		}
	}

	/* open a socket, and start handling requests */
	int fd = open_server_socket(port);
	handle_requests(fd, file_server, lru_size, multithread);

	exit(0);
}
