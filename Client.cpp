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

//sends:
//PUT <filename>\n
//<# bytes>\n
//<file contents>\n
void put_file(int fd, char *put_name, int enc)
{
	
	//error case
	if(!file_exists(put_name)){
		die("PUT", "does not exist\n");
	}
    FILE* sendptr = fopen(put_name, "rb");

	size_t sendmsgsize = 4;
	sendmsgsize += sizeof(char*)*strlen(put_name);
	size_t sendfilesize = get_size(put_name);
	sendmsgsize += digitcount(sendfilesize);
	sendmsgsize += sendfilesize;
	char sendmsg[sendmsgsize];
	bzero(sendmsg, sendmsgsize);

	//PUT <filename>\n
	strcat(sendmsg, "PUT ");
	strcat(sendmsg, put_name);
	strcat(sendmsg, "\n");

	//file contents
	char* contentstr = (char*)malloc(sizeof(char*)*sendfilesize);
	for(int i=0;i<sendfilesize;i++){
		fread(contentstr+i, 1, 1, sendptr);
	}

	//<# bytes>\n
	char sizestr[digitcount(sendfilesize)];
	sprintf(sizestr,"%d",sendfilesize);
	strcat(sendmsg, sizestr);
	strcat(sendmsg, "\n");
	
	//<file contents>\n
	strcat(sendmsg, contentstr);
	strcat(sendmsg, "\n");
	
	/*EXAMPLE ENCRYPTION SHIT*/
	// unsigned char* encrypted;
	// unsigned char* unencrypted;
	// //example encryption
	// unsigned char* from = (unsigned char*)contentstr;
	// //unsigned char from[] = "123456789123456789";
	// fprintf(stderr, "UNENCRYPTED FILE CONTENTS: %s\n", from);
	// FILE *pubptr = fopen("public.pem", "r");
	// RSA* temp = RSA_new();
	// RSA* pub = PEM_read_RSA_PUBKEY(pubptr, &temp, NULL, NULL);
	// int esize = RSA_size(temp);
	// int flen = sendfilesize;
	// encrypted = (unsigned char*)malloc(esize);
	// int er = RSA_public_encrypt(flen, from, encrypted, temp, RSA_PKCS1_OAEP_PADDING);
	// fprintf(stderr, "ESIZE: %d ENC RES: %d\n",esize,er);
	// fprintf(stderr, "ENCRYPTED FILE CONTENTS: %s\n", encrypted);

	// //example decryption
	// unsigned char* result = (unsigned char*)malloc(esize);
	// FILE *privptr = fopen("private.pem", "r");
	// RSA* utemp = RSA_new();
	// RSA* priv = PEM_read_RSAPrivateKey(privptr, &utemp, NULL, NULL);
	// int dr = RSA_private_decrypt(esize, encrypted, result, utemp, RSA_PKCS1_OAEP_PADDING);
	// fprintf(stderr, "DECRYPT RESULT %d \n", dr);
	// fprintf(stderr, "DECRYPTED FILE CONTENTS: %s\n", result);


	//send the PUT message
	write(fd, sendmsg, strlen(sendmsg));
}

//sends:
//PUTC <filename>\n
//<checksum>\n
//<# bytes>\n
//<file contents>\n
void putc_file(int fd, char *put_name, int enc){

	//error case
	if(!file_exists(put_name)){
		die("PUTC", "does not exist\n");
	}
    FILE* sendptr = fopen(put_name, "rb");

	//calculate message size
	size_t sendmsgsize = 5;
	sendmsgsize += sizeof(char*)*strlen(put_name);
	size_t sendfilesize = get_size(put_name);
	sendmsgsize += digitcount(sendfilesize);
	sendmsgsize += sendfilesize;
	char sendmsg[sendmsgsize];
	bzero(sendmsg, sendmsgsize);

	//PUT <filename>\n
	strcat(sendmsg, "PUTC ");
	strcat(sendmsg, put_name);
	strcat(sendmsg, "\n");

	//<# bytes>\n
	char sizestr[digitcount(sendfilesize)];
	sprintf(sizestr,"%d",sendfilesize);
	strcat(sendmsg, sizestr);
	strcat(sendmsg, "\n");
	
	//get file contents
	char* contentstr = (char*)malloc(sizeof(char*)*sendfilesize);
	for(int i=0;i<sendfilesize;i++){
		fread(contentstr+i, 1, 1, sendptr);
	}
	rewind(sendptr);

	//get checksum
	unsigned char digest[16];
	char* data = contentstr;
	int read_bytes;
	MD5_CTX context;
	MD5_Init(&context);
    MD5_Update(&context, data, sendfilesize-1);
	
	MD5_Final(digest, &context);

	//<checksum>\n
	char md5string[32];
	for(int i=0; i<16; i++){
		sprintf(md5string, "%02x", digest[i]);
		strcat(sendmsg, md5string);
	}
	strcat(sendmsg, "\n");

	//<file contents>\n
	strcat(sendmsg, contentstr);
	strcat(sendmsg, "\n");

	//send the PUT message
	write(fd, sendmsg, strlen(sendmsg));
}

//sends:
//GET <filename>\n
void get_file(int fd, char *get_name, char *save_name, int enc)
{
	//send message
	size_t getmsgsize = 4;
	getmsgsize += sizeof(char*)*strlen(get_name);
	char getmsg[getmsgsize];
	bzero(getmsg,getmsgsize);
	strcat(getmsg, "GET ");
	strcat(getmsg, get_name);
	strcat(getmsg, "\n");
	write(fd, getmsg, strlen(getmsg));

	//receive response from server
	const int MAXLINE = 8192;
	char buf[MAXLINE];
	bzero(buf, MAXLINE);
	server_response(fd, buf);

	//recreate the returned file if OKC
	if(strncmp(buf, "OK ", 3)!=0){
		die("GET ERROR", buf);
	}
	//parse out file name
	char* endname = strstr(buf, "\n");
	char* begname = buf+3;
	int len = endname-begname;
	char filename[len+1];
	bzero(filename, len+1);
	strncpy(filename, begname, len);
	filename[len+1] = '\0';

	//account for alternate save name
	if(save_name){
		//bzero(filename, len+2);
		filename[strlen(save_name)];
		strncpy(filename, save_name, strlen(save_name));
	}

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

	//isolate the file data
	begname = endname+1;
	endname = begname+numbytes;
	len = endname-begname;
	char filedata[len+1];
	bzero(filedata, len+1);
	strncpy(filedata, begname, len);
	filedata[len] = '\0';

	//write file
	int writefd = open(filename, O_RDWR | O_TRUNC | O_CREAT, 0777);
	write(writefd, filedata, len);
	close(writefd);
	printf("OK\n");
}

//sends:
//GETC <filename>\n
void getc_file(int fd, char *get_name, char *save_name, int enc){

	//send message
	size_t getmsgsize = 5;
	getmsgsize += sizeof(char*)*strlen(get_name);
	char getmsg[getmsgsize];
	bzero(getmsg,getmsgsize);
	strcat(getmsg, "GETC ");
	strcat(getmsg, get_name);
	strcat(getmsg, "\n");
	write(fd, getmsg, strlen(getmsg));

	//get server response and store it in buf
	const int MAXLINE = 8192;
	char buf[MAXLINE];
	bzero(buf, MAXLINE);
	server_response(fd, buf);

	//recreate the returned file if OKC
	if(strncmp(buf, "OKC ", 4)!=0){
		die("GETC ERROR", buf);
	}

	//parse out file name
	char* endname = strstr(buf, "\n");
	char* begname = buf+4;
	int len = endname-begname;
	char filename[len+1];
	bzero(filename, len+1);
	strncpy(filename, begname, len);
	filename[len+1] = '\0';

	//account for alternate save name
	if(save_name){
		filename[strlen(save_name)];
		strncpy(filename, save_name, strlen(save_name));
	}

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

	//see if checksum matches
	unsigned char digest[16];
	char* data = filedata;
	int read_bytes;
	MD5_CTX context;
	MD5_Init(&context);
	MD5_Update(&context, data, len-1);

	MD5_Final(digest, &context);

	bool hashmatch = false;
	char md5string[32];
	for(int i=0; i<16; i++){
		sprintf(md5string, "%02x", digest[i]);
		if(strncmp(md5string, checksum+(2*i), 2) == 0){
			hashmatch = true;
		}
		else{
			die("GETC ERROR", "non matching checksums");
		}
	}

	//write file
	int writefd = open(filename, O_RDWR | O_TRUNC | O_CREAT, 0777);
	write(writefd, filedata, len);
	close(writefd);
	printf("OK\n");
}

//waits for server response and stores in buf
void server_response(int fd, char* buf){

	while(1)
	{
		/* send keystrokes to the server, handling short counts */
		size_t n = strlen(buf);
		size_t nremain = n;
		ssize_t nsofar;
		char *bufp = buf;

		/* read input back from socket (again, handle short counts)*/
		bzero(buf, 8192);
		bufp = buf;
		nremain = 8192;
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
		break;
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
	int checksum = 0;
	int encryption = 0;

	check_team(argv[0]);

	/* parse the command-line options. */
	while((opt = getopt(argc, argv, "hs:P:G:S:p:ae")) != -1)
	{
		switch(opt)
		{
		case 'h': help(argv[0]); break;
		case 's': server = optarg; break;
		case 'P': put_name = optarg; break;
		case 'G': get_name = optarg; break;
		case 'S': save_name = optarg; break;
		case 'p': port = atoi(optarg); break;
		case 'a': checksum = 1; break;
		case 'e': encryption = 1; break;
		}
	}


	/* open a connection to the server */
	int fd = connect_to_server(server, port);

	/* put or get, as appropriate */
	if(put_name && checksum==1){
	  	putc_file(fd, put_name, encryption);
	} else if(put_name && checksum == 0){
		put_file (fd, put_name, encryption);
	}
	else if(get_name && checksum ==1){
		getc_file(fd, get_name, save_name, encryption);
	}
	else{
		get_file(fd, get_name, save_name, encryption);
	}

	/* close the socket */
	int rc;
	if((rc = close(fd)) < 0)
	{
		die("Close error: ", strerror(errno));
	}
	exit(0);
}
