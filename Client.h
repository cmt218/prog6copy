#pragma once

/*
 * help() - Print a help message
 */
void help(char *progname);

/*
 * die() - print an error and exit the program
 */
void die(const char *msg1, const char *msg2);

/*
 * connect_to_server() - open a connection to the server specified by the
 *                       parameters
 */
int connect_to_server(char *server, int port);

/*
 * echo_client() - this is dummy code to show how to read and write on a
 *                 socket when there can be short counts.  The code
 *                 implements an "echo" client.
 */
void echo_client(int fd);

/*
 * put_file() - send a file to the server accessible via the given socket fd
 */
void put_file(int fd, char *put_name);

/*
 * get_file() - get a file from the server accessible via the given socket
 *              fd, and save it according to the save_name
 */
void get_file(int fd, char *get_name, char *save_name);

/*cmt218
 *file_exists() - check if a file with name exists in current directory
 *
 */
bool file_exists(char *name);

/*cmt218
 *get_size() - return the size of some file in bytes
 *
 */
size_t get_size(char *name);

size_t getintstringlen(int size);

void putc_file(int fd, char *put_name);

void compute_checksum(char* contentstr, int sendfilesize);