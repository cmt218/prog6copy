#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/ssl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "support.h"
#include "Client.h"

void help(char *progname)
{
	printf("Usage: %s [OPTIONS]\n", progname);
	printf("Perform a PUT or a GET from a network file server\n");
	printf("  -P    PUT file indicated by parameter\n");
	printf("  -G    GET file indicated by parameter\n");
	printf("  -s    server info (IP or hostname)\n");
	printf("  -p    port on which to contact server\n");
	printf("  -S    for GETs, name to use when saving file locally\n");
}

void die(const char *msg1, const char *msg2)
{
	fprintf(stderr, "%s, %s\n", msg1, msg2);
	exit(0);
}

/*
 * connect_to_server() - open a connection to the server specified by the
 *                       parameters
 */
int connect_to_server(char *server, int port)
{
	int clientfd;
	struct hostent *hp;
	struct sockaddr_in serveraddr;
	char errbuf[256];                                   /* for errors */

	/* create a socket */
	if((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		die("Error creating socket: ", strerror(errno));
	}

	/* Fill in the server's IP address and port */
	if((hp = gethostbyname(server)) == NULL)
	{
		sprintf(errbuf, "%d", h_errno);
		die("DNS error: DNS error ", errbuf);
	}
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)hp->h_addr_list[0], (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
	serveraddr.sin_port = htons(port);

	/* connect */
	if(connect(clientfd, (struct sockaddr *) &serveraddr, sizeof(serveraddr)) < 0)
	{
		die("Error connecting: ", strerror(errno));
	}
	return clientfd;
}

/*
 * echo_client() - this is dummy code to show how to read and write on a
 *                 socket when there can be short counts.  The code
 *                 implements an "echo" client.
 */
void echo_client(int fd)
{
	// main loop
	while(1)
	{
		/* set up a buffer, clear it, and read keyboard input */
		const int MAXLINE = 8192;
		char buf[MAXLINE];
		bzero(buf, MAXLINE);
		if(fgets(buf, MAXLINE, stdin) == NULL)
		{
			if(ferror(stdin))
			{
				die("fgets error", strerror(errno));
			}
			break;
		}

		/* send keystrokes to the server, handling short counts */
		size_t n = strlen(buf);
		size_t nremain = n;
		ssize_t nsofar;
		char *bufp = buf;
		while(nremain > 0)
		{
			if((nsofar = write(fd, bufp, nremain)) <= 0)
			{
				if(errno != EINTR)
				{
					fprintf(stderr, "Write error: %s\n", strerror(errno));
					exit(0);
				}
				nsofar = 0;
			}
			nremain -= nsofar;
			bufp += nsofar;
		}

		/* read input back from socket (again, handle short counts)*/
		bzero(buf, MAXLINE);
		bufp = buf;
		nremain = MAXLINE;
		while(1)
		{
			if((nsofar = read(fd, bufp, nremain)) < 0)
			{
				if(errno != EINTR)
				{
					die("read error: ", strerror(errno));
				}
				continue;
			}
			/* in echo, server should never EOF */
			if(nsofar == 0)
			{
				die("Server error: ", "received EOF");
			}
			bufp += nsofar;
			nremain -= nsofar;
			if(*(bufp-1) == '\n')
			{
				*bufp = 0;
				break;
			}
		}

		/* output the result */
		printf("%s", buf);
	}
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
 *get_size() - return the size of a file in bytes
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

size_t getintstringlen(int size){
	size_t l = 1;
	int cpy = size;
	while(cpy > 9){
		l++;
		cpy /= 10;
	}
}

/*
 * put_file() - send a file to the server accessible via the given socket fd
 */
void put_file(int fd, char *put_name)
{
	/* TODO: implement a proper solution, instead of calling the echo() client */	
	
	if(file_exists(put_name)){
		//initially 10 to account for 'PUT' and new line characters being sent
		size_t sendmsgsize = 6;
		//add size of file name
		sendmsgsize += sizeof(char*)*strlen(put_name);
		//add size of byte number
		size_t sendfilesize = get_size(put_name);
		sendmsgsize += sendfilesize/10;
		//add size of file
		sendmsgsize += sendfilesize;


		//begin building the client's PUT message
		char sendmsg[sendmsgsize];
		bzero(sendmsg, sendmsgsize);

		//PUT <filename>\n
		strcat(sendmsg, "PUT ");
		strcat(sendmsg, put_name);
		strcat(sendmsg, "\n");

		//<# bytes>\n
		//int sizelen = getintstringlen(sendfilesize);
		char sizestr[sendmsgsize/10];
		sprintf(sizestr,"%d",sendfilesize);
		strcat(sendmsg, sizestr);
		strcat(sendmsg, "\n");
		
		//<file contents>\n
		char* contentstr = (char*)malloc(sizeof(char*)*sendfilesize);
		FILE* sendptr = fopen(put_name, "r");
		for(int i=0;i<sendfilesize;i++){
			fread(contentstr+i, 1, 1, sendptr);
		}
		strcat(sendmsg, contentstr);
		strcat(sendmsg, "\n");

		//send the PUT message
		write(fd, sendmsg, strlen(sendmsg));
		
	}
	else{
		die("File Error: ", "file does not exist");
	}
	
}

/*
 * get_file() - get a file from the server accessible via the given socket
 *              fd, and save it according to the save_name
 */
void get_file(int fd, char *get_name, char *save_name)
{
	/* TODO: implement a proper solution, instead of calling the echo() client */
	size_t getmsgsize = 6;
	getmsgsize += sizeof(char*)*strlen(get_name);
	char getmsg[getmsgsize];
	bzero(getmsg,getmsgsize);
	strcat(getmsg, "GET ");
	strcat(getmsg, get_name);
	strcat(getmsg, "\n");
	write(fd, getmsg, strlen(getmsg));

	
	//receive response from server
	/* set up a buffer, clear it, and read keyboard input */
	const int MAXLINE = 8192;
	char buf[MAXLINE];
	bzero(buf, MAXLINE);

	while(1)
	{

		/* send keystrokes to the server, handling short counts */
		size_t n = strlen(buf);
		size_t nremain = n;
		ssize_t nsofar;
		char *bufp = buf;

		/* read input back from socket (again, handle short counts)*/
		bzero(buf, MAXLINE);
		bufp = buf;
		nremain = MAXLINE;
		while(1)
		{
			if((nsofar = read(fd, bufp, nremain)) < 0)
			{
				if(errno != EINTR)
				{
					die("read error: ", strerror(errno));
				}
				continue;
			}
			/* in echo, server should never EOF */
			if(nsofar == 0)
			{
				die("Server error: ", "received EOF");
			}
			bufp += nsofar;
			nremain -= nsofar;
			if(*(bufp-1) == '\n')
			{
				*bufp = 0;
				break;
			}
		}

		/* output the result */
		//printf("%s", buf);
		break;
	}

	//create the new file in directory
	//parse out file name
	char* endname = strstr(buf, "\n");
	char* begname = buf+3;
	int len = endname-begname;
	char filename[len+2];
	bzero(filename, len+2);
	strncpy(filename, begname, len);
	filename[len+1] = '\0';

	//fprintf(stderr, "SAVE NAME: %s \n", save_name);

	if(save_name){
		//bzero(filename, len+2);
		filename[strlen(save_name)];
		strncpy(filename, save_name, strlen(save_name));
	}


	//remove file if it exists because it will be overwritten
	if(file_exists(filename)){
		remove(filename);
	}
	FILE *newptr = fopen(filename, "ab+");

	//parse out bytes size
	begname = endname+1;
	endname = strstr(begname, "\n");
	len = endname-begname;
	int numbytes;
	char numbytesstring[len+2];
	bzero(numbytesstring, len+2);
    strncpy(numbytesstring, begname, len);
	numbytesstring[len+1] = '\0';
	sscanf(numbytesstring, "%d", &numbytes);

	//expand the file to our needs
	fseek(newptr, numbytes, SEEK_SET);
	//fseek(newptr, 0, SEEK_SET);

	//isolate the file data
	begname = endname+1;
	endname = strstr(begname, "\n");
	len = endname-begname;
	char filedata[len+2];
	bzero(filedata, len+2);
	strncpy(filedata, begname, len);
	filedata[len] = '\0';

	int writefd = fileno(newptr);
	write(writefd, filedata, len);

}



/*
 * main() - parse command line, open a socket, transfer a file
 */
int main(int argc, char **argv)
{
	/* for getopt */
	long  opt;
	char *server = NULL;
	char *put_name = NULL;
	char *get_name = NULL;
	int   port;
	char *save_name = NULL;

	check_team(argv[0]);

	/* parse the command-line options. */
	while((opt = getopt(argc, argv, "hs:P:G:S:p:")) != -1)
	{
		switch(opt)
		{
		case 'h': help(argv[0]); break;
		case 's': server = optarg; break;
		case 'P': put_name = optarg; break;
		case 'G': get_name = optarg; break;
		case 'S': save_name = optarg; break;
		case 'p': port = atoi(optarg); break;
		}
	}

	/* open a connection to the server */
	int fd = connect_to_server(server, port);

	/* put or get, as appropriate */
	if(put_name)
	{
		put_file(fd, put_name);
	}
	else
	{
		get_file(fd, get_name, save_name);
	}

	/* close the socket */
	int rc;
	if((rc = close(fd)) < 0)
	{
		die("Close error: ", strerror(errno));
	}
	exit(0);
}
