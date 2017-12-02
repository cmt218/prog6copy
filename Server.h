#pragma once

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
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "support.h"

/*
 * help() - Print a help message
 */
void help(char *progname);

/*
 * die() - print an error and exit the program
 */
void die(const char *msg1, char *msg2);

/*
 * open_server_socket() - Open a listening socket and return its file
 *                        descriptor, or terminate the program
 */

int open_server_socket(int port);

/*
 * handle_requests() - given a listening file descriptor, continually wait
 *                     for a request to come in, and when it arrives, pass it
 *                     to service_function.  Note that this is not a
 *                     multi-threaded server.
 */
void handle_requests(int listenfd, void (*service_function)(int, int), int param, bool multithread);

/*
 * file_server() - Read a request from a socket, satisfy the request, and
 *                 then close the connection.
 */
void file_server(int connfd, int lru_size);

bool file_exists(char *name);

/*
 * put_file() - put a file in the server's directory
 *
 */
void put_file(char* putmsg);

/*
 * get_file() - return a file to the client
 *
 */
void get_file(int connfd, char* getmsg);

size_t get_size(char *name);

size_t getintstringlen(int size);

void putc_file(char* putmsg);